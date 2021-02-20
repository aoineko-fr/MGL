@echo on
set CMSXimg=..\..\tools\CMSXimg\CMSXimg.exe

echo ---- BUILD FONT DATA ----
REM %CMSXimg% -in font\misc\acme_5_outlines_font_gb.png -out font_acme.h -pos 0 0 -size 8 8 -num 16 4 -trans 0xFFFFFF -bpc 1 -name g_Font_Acme 					-skip -font 8 8 ! _
%CMSXimg% font\font_cmsx_big1.png     -out ..\src\font\font_cmsx_big1.h     -pos 0 0   -size 8 11 -gap 0 5  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Big1     -copy cmsx.txt -skip  -font 8 11 ! _
%CMSXimg% font\font_cmsx_curs1.png    -out ..\src\font\font_cmsx_curs1.h    -pos 0 16  -size 8 8  -gap 0 0  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Curs1    -copy cmsx.txt -skip  -font 8 8 ! ~
%CMSXimg% font\font_cmsx_curs1b.png   -out ..\src\font\font_cmsx_curs1b.h   -pos 0 16  -size 8 8  -gap 0 0  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Curs1B   -copy cmsx.txt -skip  -font 8 8 ! ~
%CMSXimg% font\font_cmsx_digit1.png   -out ..\src\font\font_cmsx_digit1.h   -pos 0 24  -size 8 8  -gap 0 0  -num 16 3  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Digit1   -copy cmsx.txt        -font 6 8 0 _
%CMSXimg% font\font_cmsx_digit2.png   -out ..\src\font\font_cmsx_digit2.h   -pos 0 24  -size 8 8  -gap 0 0  -num 16 3  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Digit2   -copy cmsx.txt        -font 6 8 0 _
%CMSXimg% font\font_cmsx_mini1.png    -out ..\src\font\font_cmsx_mini1.h    -pos 0 16  -size 8 5  -gap 0 3  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Mini1    -copy cmsx.txt -skip  -font 4 6 ! ~
%CMSXimg% font\font_cmsx_mini1b.png   -out ..\src\font\font_cmsx_mini1b.h   -pos 0 16  -size 8 5  -gap 0 3  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Mini1B   -copy cmsx.txt -skip  -font 4 6 ! ~
%CMSXimg% font\font_cmsx_mini2.png    -out ..\src\font\font_cmsx_mini2.h    -pos 0 16  -size 8 6  -gap 0 2  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Mini2    -copy cmsx.txt -skip  -font 5 6 ! ~
%CMSXimg% font\font_cmsx_mini3.png    -out ..\src\font\font_cmsx_mini3.h    -pos 0 16  -size 8 6  -gap 0 2  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Mini3    -copy cmsx.txt -skip  -font 5 6 ! ~
%CMSXimg% font\font_cmsx_neon1.png    -out ..\src\font\font_cmsx_neon1.h    -pos 0 16  -size 8 8  -gap 0 0  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Neon1    -copy cmsx.txt -skip  -font 8 8 ! ~
%CMSXimg% font\font_cmsx_neon1b.png   -out ..\src\font\font_cmsx_neon1b.h   -pos 0 16  -size 8 8  -gap 0 0  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Neon1B   -copy cmsx.txt -skip  -font 8 8 ! ~
%CMSXimg% font\font_cmsx_neon2.png    -out ..\src\font\font_cmsx_neon2.h    -pos 0 16  -size 8 8  -gap 0 0  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Neon2    -copy cmsx.txt -skip  -font 8 8 ! ~
%CMSXimg% font\font_cmsx_rune2.png    -out ..\src\font\font_cmsx_rune2.h    -pos 0 16  -size 8 8  -gap 0 0  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Rune2    -copy cmsx.txt -skip  -font 8 8 ! _
%CMSXimg% font\font_cmsx_rune2b.png   -out ..\src\font\font_cmsx_rune2b.h   -pos 0 16  -size 8 8  -gap 0 0  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Rune2B   -copy cmsx.txt -skip  -font 8 8 ! _
%CMSXimg% font\font_cmsx_std0.png     -out ..\src\font\font_cmsx_std0.h     -pos 0 16  -size 8 8  -gap 0 0  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Std0     -copy cmsx.txt -skip  -font 6 8 ! ~
%CMSXimg% font\font_cmsx_std1.png     -out ..\src\font\font_cmsx_std1.h     -pos 0 16  -size 8 8  -gap 0 0  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Std1     -copy cmsx.txt -skip  -font 6 8 ! ~
%CMSXimg% font\font_cmsx_std2.png     -out ..\src\font\font_cmsx_std2.h     -pos 0 16  -size 8 8  -gap 0 0  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Std2     -copy cmsx.txt -skip  -font 6 8 ! ~
%CMSXimg% font\font_cmsx_std3.png     -out ..\src\font\font_cmsx_std3.h     -pos 0 16  -size 8 7  -gap 0 1  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_CMSX_Std3     -copy cmsx.txt -skip  -font 6 8 ! }
%CMSXimg% font\font_cmsx_symbol1.png  -out ..\src\font\font_cmsx_symbol1.h  -pos 0 0   -size 8 8  -gap 0 0  -num 16 14 -trans 0x000000  -bpc 1  -name g_Font_CMSX_Symbol1  -copy cmsx.txt        -font 8 8 0x00 0xE0
%CMSXimg% font\font_tsm9900.png       -out ..\src\font\font_tsm9900.h       -pos 0 16  -size 8 8  -gap 0 0  -num 16 6  -trans 0x000000  -bpc 1  -name g_Font_TMS9900       -copy          -skip  -font 6 8 ! ~
REM %CMSXimg% -in font\font_darkrose.png	-out font_darkrose.h    -pos 0 16 -size 8 8  -gap 0 0 -num 16 6  -trans 0x000000 -bpc 1 -name g_Font_Darkrose		-skip -font 8 8 ! ~
REM %CMSXimg% -in font\IBMFont.bmp			-out font_ibm.h         -pos 0 0  -size 8 8  -gap 0 0 -num 16 16 -trans 0x040404 -bpc 1 -name g_Font_IBM			-skip -font 8 8 0x01 0xFE
REM %CMSXimg% -in font\oxygene.png			-out font_oxygene.h     -pos 0 16 -size 8 8  -gap 0 0 -num 16 6  -trans 0x000000 -bpc 1 -name g_Font_Oxygene		-skip -font 8 8 ! _

echo ---- BUILD CURSOR DATA ----
%CMSXimg% cursor\cur_cmsx1.png        -out ..\src\cursor\cur_cmsx1.h        -pos 0 0   -size 8 8  -gap 0 0  -num 16 2  -trans 0x000000  -bpc 1  -name g_Cursor_CMSX1       -copy cmsx.txt 

PAUSE