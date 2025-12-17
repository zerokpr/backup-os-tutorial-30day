TOOLPATH = ../z_tools/
INCPATH  = ../z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = copy
DEL      = del

# デフォルト動作
default :
	$(MAKE) haribote.img

# ファイル生成規則
haribote.img : haribote/ipl20.bin haribote/haribote.sys make.bat hello/hello.hrb \
		a/a.hrb winhelo/winhelo.hrb stars/stars.hrb lines/lines.hrb walk/walk.hrb \
		noodle/noodle.hrb beepdown/beepdown.hrb color/color.hrb color2/color2.hrb \
		crack7/crack7.hrb sosu/sosu.hrb type/type.hrb iroha/iroha.hrb chklang/chklang.hrb \
		notrec/notrec.hrb bball/bball.hrb invader/invader.hrb calc/calc.hrb tview/tview.hrb \
		gview/gview.hrb \
		Makefile
	$(EDIMG)	imgin:../z_tools/fdimg0at.tek \
		wbinimg src:haribote/ipl20.bin len:512 from:0 to:0 \
		copy from:haribote/haribote.sys to:@: \
		copy from:haribote/ipl20.nas to:@: \
		copy from:make.bat to:@: \
		copy from:hello/hello.hrb to:@: \
		copy from:a/a.hrb to:@: \
		copy from:winhelo/winhelo.hrb to:@: \
		copy from:lines/lines.hrb to:@: \
		copy from:walk/walk.hrb to:@: \
		copy from:stars/stars.hrb to:@: \
		copy from:noodle/noodle.hrb to:@: \
		copy from:beepdown/beepdown.hrb to:@: \
		copy from:color/color.hrb to:@: \
		copy from:color2/color2.hrb to:@: \
		copy from:crack7/crack7.hrb to:@: \
		copy from:sosu/sosu.hrb to:@: \
		copy from:type/type.hrb to:@: \
		copy from:iroha/iroha.hrb to:@: \
		copy from:chklang/chklang.hrb to:@: \
		copy from:notrec/notrec.hrb to:@: \
		copy from:bball/bball.hrb to:@: \
		copy from:invader/invader.hrb to:@: \
		copy from:calc/calc.hrb to:@: \
		copy from:tview/tview.hrb to:@: \
		copy from:gview/gview.hrb to:@: \
		copy from:haribote/roachan.jpg to:@: \
		copy from:pictdata/roachan2.jpg to:@: \
		copy from:pictdata/fujisan.jpg to:@: \
		copy from:pictdata/night.bmp to:@: \
		copy from:haribote/jis.txt to:@: \
		copy from:haribote/euc.txt to:@: \
		copy from:nihongo/nihongo.fnt to:@: \
		imgout:haribote.img

run :
	$(MAKE) haribote.img
	$(COPY) haribote.img ..\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ../z_tools/qemu

install :
	$(MAKE) haribote.img
	$(IMGTOL) w a: haribote.img

full : 
	$(MAKE) -C haribote
	$(MAKE) -C corosapi
	$(MAKE) -C a
	$(MAKE) -C beepdown
	$(MAKE) -C color
	$(MAKE) -C color2
	$(MAKE) -C crack7
	$(MAKE) -C hello
	$(MAKE) -C lines
	$(MAKE) -C noodle
	$(MAKE) -C stars
	$(MAKE) -C walk
	$(MAKE) -C winhelo
	$(MAKE) -C sosu
	$(MAKE) -C type
	$(MAKE) -C iroha
	$(MAKE) -C chklang
	$(MAKE) -C notrec
	$(MAKE) -C bball
	$(MAKE) -C invader
	$(MAKE) -C calc
	$(MAKE) -C tview
	$(MAKE) -C gview
	$(MAKE) haribote.img

run_full :
	$(MAKE) full
	$(COPY) haribote.img ..\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ../z_tools/qemu

install_full :
	$(MAKE) full
	$(IMGTOL) w a: haribote.img

run_os :
	$(MAKE) -C haribote
	$(MAKE) run

clean :
# 何もしない

src_only :
	$(MAKE) clean
	-$(DEL) haribote.img

clean_full :
	$(MAKE) -C haribote		clean
	$(MAKE) -C corosapi		clean
	$(MAKE) -C a			clean
	$(MAKE) -C beepdown		clean
	$(MAKE) -C color		clean
	$(MAKE) -C color2		clean
	$(MAKE) -C crack7		clean
	$(MAKE) -C hello		clean
	$(MAKE) -C lines		clean
	$(MAKE) -C noodle		clean
	$(MAKE) -C stars		clean
	$(MAKE) -C walk			clean
	$(MAKE) -C winhelo		clean
	$(MAKE) -C sosu			clean
	$(MAKE) -C type			clean
	$(MAKE) -C iroha		clean
	$(MAKE) -C chklang		clean
	$(MAKE) -C notrec		clean
	$(MAKE) -C bball		clean
	$(MAKE) -C invader		clean
	$(MAKE) -C calc			clean
	$(MAKE) -C tview		clean
	$(MAKE) -C gview		clean

src_only_full :
	$(MAKE) -C haribote		src_only
	$(MAKE) -C corosapi		src_only
	$(MAKE) -C a			src_only
	$(MAKE) -C beepdown		src_only
	$(MAKE) -C color		src_only
	$(MAKE) -C color2		src_only
	$(MAKE) -C crack7		src_only
	$(MAKE) -C hello		src_only
	$(MAKE) -C lines		src_only
	$(MAKE) -C noodle		src_only
	$(MAKE) -C stars		src_only
	$(MAKE) -C walk			src_only
	$(MAKE) -C winhelo		src_only
	$(MAKE) -C sosu			src_only
	$(MAKE) -C type			src_only
	$(MAKE) -C iroha		src_only
	$(MAKE) -C chklang		src_only
	$(MAKE) -C notrec		src_only
	$(MAKE) -C bball		src_only
	$(MAKE) -C invader		src_only
	$(MAKE) -C calc			src_only
	$(MAKE) -C tview		src_only
	$(MAKE) -C gview		src_only
	$(DEL) haribote.img

refresh :
	$(MAKE) full
	$(MAKE) clean_full
	-$(DEL) haribote.img
