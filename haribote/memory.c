#include "bootpack.h"

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

/**
 * 4kバイトに切り上げてメモリを確保する
 */
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size) {
	unsigned int ret;
	size = (size + 0xfff) & 0xfffff000;
	ret = memman_alloc(man, size);
	return ret;
}

/**
 * 4kバイトに切り上げてメモリを解放する
 */
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size) {
	int ret;
	size = (size + 0xfff) & 0xfffff000;
	ret = memman_free(man, addr, size);
	return ret;
}

unsigned int memtest(unsigned int start, unsigned int end) {
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	/** 
	 * 386か486の確認
	 * 386の場合はAC-bitを1にして保存しても0になるため、保存した後に呼び出してみることで確認できる
	*/
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT;
	io_store_eflags(eflg);
	eflg = io_load_eflags();

	if ((eflg & EFLAGS_AC_BIT) != 0) flg486 = 1;

	eflg &= ~EFLAGS_AC_BIT;
	io_store_eflags(eflg);
	
	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; // キャッシュ禁止
		store_cr0(cr0);
	}

	// メモリチェック本体
	i = memtest_sub(start, end);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE;	// キャッシュ許可
		store_cr0(cr0);
	}

	return i;
}



void memman_init(struct MEMMAN *man) {
	man->frees = 0;	// 空き情報の個数
	man->maxfrees = 0;
	man->lostsize = 0;	// 解放に失敗した合計サイズ()
	man->losts = 0;		// 解放に失敗した回数
	return ;
}

/**
 * 空きメモリの総量を返す
 */
unsigned int memman_total(struct MEMMAN *man) {
	unsigned int i, ret = 0;
	for (i = 0; i < man->frees; ++i) ret += man->free[i].size;
	return ret;
}

/**
 * メモリ確保
 * man: メモリマネージャ
 * size: 確保したいメモリのサイズ
 * 戻り値: 確保したメモリの先頭のアドレス
 */
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size) {
	unsigned int i, addr;
	for (i = 0; i < man->frees; ++i) {
		if (man->free[i].size >= size) {
			addr = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;

			if (man->free[i].size == 0) { // free[i]がなくなったので前に詰める
				--(man->frees);
				for (; i < man->frees; ++i) man->free[i] = man->free[i+1];
			}

			return addr;
		}
	}

	return 0;	// 空きがない場合
}

/**
 * メモリ開放
 */
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size) {
	int i, j;
	for (i = 0; i < man->frees; ++i) 
		if (man->free[i].addr > addr) break;

	if ( // 前後の要素とまとめる必要がある場合
		0 < i && i < man->frees
		&& man->free[i-1].addr + man->free[i-1].size == addr
		&& man->free[i].addr == addr + size
	) {
		man->free[i-1].size += size + man->free[i].size;
		
		--(man->frees);
		for (j = i; j < man->frees; ++j) man->free[j] = man->free[j+1]; // free[i]の位置が1個空くので後ろ側を手前へずらしていく	

		return 0;
	} else if (0 < i && man->free[i-1].addr + man->free[i-1].size == addr) { // 前の要素とのみまとめる必要がある場合
		man->free[i-1].size += size;
		return 0;
	} else if (i < man->frees && man->free[i].addr == addr + size) { // 次の要素とのみまとめる必要がある場合
		man->free[i].addr -= size;
		man->free[i].size += size;
		return 0;
	} else {	// どれともまとめる必要がない場合
		if (man->frees < MEMMAN_FREES) {
			++(man->frees);

			// free[i]以降の要素を1つ後ろ側へずらしていく。iが末尾ならこのループはスキップされる。
			for(j = man->frees-1; j > i; --j) man->free[j] = man->free[j-1];
			man->free[i].addr = addr;
			man->free[i].size = size;
			
			if (man->maxfrees < man->frees) man->maxfrees = man->frees;
			
			return 0;
		}
	}

	// ここに来た場合、メモリ解放に失敗
	++(man->losts);
	man->lostsize += size;
	return -1;
}
