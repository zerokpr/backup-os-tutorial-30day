#include "bootpack.h"

struct TIMER *task_timer;
struct TASKCTL *taskctl;

/**
 * 現在動作中のタスクのポインタを返す
 */
struct TASK *task_now(void) {
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	return tl->tasks[tl->now];
}

/**
 * 何もしないタスク。タスク管理リストの番兵用
 */
void task_idle(void) {
	while (1) io_hlt();
}

/**
 * TASKLEVELにタスクを1つ追加する。
 * どのレベルにタスクを追加するかはタスク構造体内に情報がある
 */
void task_add(struct TASK *task) {
	struct TASKLEVEL *tl = &taskctl->level[task->level];
	// if (tl->running >= MAX_TASK_PER_LV) return 0;
	tl->tasks[tl->running++] = task;
	task->flags = TASK_FLAGS_RUNNING;
	return;
}

void task_remove(struct TASK *task) {
	int i;
	struct TASKLEVEL *tl = &taskctl->level[task->level];

	/* taskの位置を探索する */
	for (i = 0; i < tl->running; ++i) {
		if (tl->tasks[i] == task) break;
	}

	--tl->running;
	if (i < tl->now) --tl->now;
	if (tl->now >= tl->running) {
		tl->now  = 0;
	}
	task->flags = TASK_FLAGS_ALLOCATED;

	for (; i < tl->running; ++i) {
		tl->tasks[i] = tl->tasks[i+1];
	}

	return;
}

/**
 * タスクスイッチの際にどのレベルにするか確認する
 * 実行後はtaskctl->now_lvが次に変更するべきタスクレベルに更新される
 */
void task_switchsub(void) {
	int i;
	/* 一番上のレベルを探す */
	for (i = 0; i < MAX_TASKLEVELS; ++i) {
		if (taskctl->level[i].running > 0) break;
	}

	taskctl->now_lv = i;
	taskctl->lv_change = 0;
	return;
}


/**
 * タスク管理機構初期化
 */
struct TASK *task_init(struct MEMMAN *memman) {
	int i;
	struct TASK *task, *idle_task;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));
	for (i = 0; i < MAX_TASKS; ++i) {
		taskctl->tasks0[i].flags = 0;
		taskctl->tasks0[i].selector = (TASK_GDT0 + i) * 8;
		taskctl->tasks0[i].tss.ldtr = (TASK_GDT0 + MAX_TASKS + i) * 8;
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
		set_segmdesc(gdt + TASK_GDT0 + MAX_TASKS + i, 15, (int) taskctl->tasks0[i].ldt, AR_LDT);
	}

	for (i = 0; i < MAX_TASKLEVELS; ++i) {
		taskctl->level[i].running = 0;
		taskctl->level[i].now = 0;
	}

	task = task_alloc();

	task->flags = TASK_FLAGS_RUNNING;	// 動作中マーク
	task->priority = 2;					// 0.02秒
	task->level = 0;					// 最初のタスクは最高レベルにしておく
	task->langmode = 0;

	task_add(task);
	task_switchsub();	// レベル設定
	load_tr(task->selector);
	task_timer = timer_alloc();
	timer_settime(task_timer, task->priority);

	idle_task = task_alloc();
	idle_task->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	idle_task->tss.eip = (int) &task_idle;
	idle_task->tss.es = 1 * 8;
	idle_task->tss.cs = 2 * 8;
	idle_task->tss.ss = 1 * 8;
	idle_task->tss.ds = 1 * 8;
	idle_task->tss.fs = 1 * 8;
	idle_task->tss.gs = 1 * 8;
	task_run(idle_task, MAX_TASKLEVELS - 1, 1);

	return task;
}

struct TASK *task_alloc(void) {
	int i;
	struct TASK *task;
	for (i = 0; i < MAX_TASKS; ++i) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->flags = 1;
			task->tss.eflags = 0x00000202;
			task->tss.eax = 0;
			task->tss.ecx = 0;
			task->tss.edx = 0;
			task->tss.ebp = 0;
			task->tss.esi = 0;
			task->tss.edi = 0;
			task->tss.es = 0;
			task->tss.ds = 0;
			task->tss.fs = 0;
			task->tss.gs = 0;
			task->tss.iomap = 0x40000000;
			task->tss.ss0 = 0;
			return task;
		}
	}

	return 0;
}

/**
 * allocで確保したtaskを実行する
 */
void task_run(struct TASK *task, int level, int priority) {
	if (level < 0 || level >= MAX_TASKLEVELS) level = task->level;	// レベルを変更しない
	if (priority > 0) task->priority = priority;

	if (task->flags == TASK_FLAGS_RUNNING && task->level != level) {	// 動作中のレベルの変更
		task_remove(task);	// これを実行するとflagsは1になるので下のifも実行される
	}

	if (task->flags != TASK_FLAGS_RUNNING) {
		task->level = level;
		task_add(task);
	}

	taskctl->lv_change = 1;	// 次回タスクスイッチの時にレベルを見直す
	return;
}


/**
 * 
 */
void task_sleep(struct TASK *task) {
	struct TASK *now_task;
	if (task->flags == TASK_FLAGS_RUNNING) {
		now_task = task_now();
		task_remove(task);

		if (task == now_task) {
			task_switchsub();
			now_task = task_now();
			farjmp(0, now_task->selector);
		}
	}
	return;
}


/**
 * タスク切り替え
 */
void task_switch(void) {
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	struct TASK *new_task, *now_task = tl->tasks[tl->now];

	++tl->now;
	if (tl->now == tl->running) tl->now = 0;

	if (taskctl->lv_change != 0) {
		task_switchsub();
		tl = &taskctl->level[taskctl->now_lv];
	}

	new_task = tl->tasks[tl->now];
	timer_settime(task_timer, new_task->priority);

	if (new_task != now_task) farjmp(0, new_task->selector);
	
	return;
}
