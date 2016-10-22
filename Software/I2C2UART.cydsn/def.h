
#include <project.h>
#include <cytypes.h>

#ifdef	GLOBAL_VALUE_DEFINE
	#define	GLOBAL
	#define GLOBAL_VAL(v)  = (v)
#else
	#define	GLOBAL	extern
	#define GLOBAL_VAL(v)   /*  */
#endif


/* Slave address of EEPROM */
#define BUFFER_SIZE		128									// ページライト/シーケンシャルリード max 128byte
#define SUB_ADDR_SIZE	8									// サブアドレス max 3byte
#define CMD_PARA_SIZE	4 + SUB_ADDR_SIZE + BUFFER_SIZE		// コマンドパラメータのサイズ


/* I2C パラメータ  */
GLOBAL 	uint8 dev_addr GLOBAL_VAL(0x00);
GLOBAL 	uint8 buf_cnt  GLOBAL_VAL(0x00);

GLOBAL 	char *bCmdParam[CMD_PARA_SIZE];		// コマンドパラメータ配列．パラメータ数25個まで　コマンド配列をデリミッタで分けた後の配列.
GLOBAL 	char cmd_buffer[420];				// コマンド文字配列．最大420文字まで.

/* I2C buffer with the data read from EEPROM */
GLOBAL uint8	i2cBufferRead[BUFFER_SIZE];
/* I2C buffer with the data to be written to EZI2C */
GLOBAL uint8   i2cBufferWrite[SUB_ADDR_SIZE + BUFFER_SIZE];	// 書き込み用バッファ サイズはサブアドレスサイズ と バッファサイズ

GLOBAL char8 cha[2] 		GLOBAL_VAL("\0");			// それで文字列を作る
GLOBAL char8 tmp_chr[100] 	GLOBAL_VAL("\0");

//***************************************************************************
// keilでコンパイルするときは、変数名の cmd_s と cmd_S の区別がつかず、linkerでエラーが出る。
// なので、変数名を、cmd_S から cmd_ssに変更して対応する。
//***************************************************************************

GLOBAL const char	cmd_s[2]  	GLOBAL_VAL("s\0");	// スタートビットコマンド
GLOBAL const char	cmd_ss[2]  	GLOBAL_VAL("S\0");	// スタートビットコマンド
GLOBAL const char	cmd_p[2]  	GLOBAL_VAL("p\0");	// ストップビットコマンド
GLOBAL const char	cmd_pp[2]  	GLOBAL_VAL("P\0");	// ストップビットコマンド
GLOBAL const char	cmd_w[2]  	GLOBAL_VAL("w\0");	// バイト/ページ 書き込みコマンド
GLOBAL const char	cmd_ww[2]  	GLOBAL_VAL("W\0");	// バイト/ページ 書き込みコマンド
GLOBAL const char	cmd_w1[3]  	GLOBAL_VAL("w1\0");	// ランダム読み込み用 サブアドレス 1byte 書き込み
GLOBAL const char	cmd_ww1[3]  GLOBAL_VAL("W1\0");	// ランダム読み込み用 サブアドレス 1byte 書き込み
GLOBAL const char	cmd_w2[3]  	GLOBAL_VAL("w2\0");	// ランダム読み込み用 サブアドレス 2byte 書き込み
GLOBAL const char	cmd_ww2[3]  GLOBAL_VAL("W2\0");	// ランダム読み込み用 サブアドレス 2byte 書き込み
GLOBAL const char	cmd_w3[3]  	GLOBAL_VAL("w3\0");	// ランダム読み込み用 サブアドレス 3byte 書き込み
GLOBAL const char	cmd_ww3[3]  GLOBAL_VAL("W3\0");	// ランダム読み込み用 サブアドレス 3byte 書き込み
GLOBAL const char	cmd_w4[4]  	GLOBAL_VAL("w4\0");	// ランダム読み込み用 サブアドレス 4byte 書き込み
GLOBAL const char	cmd_ww4[4]  GLOBAL_VAL("W4\0");	// ランダム読み込み用 サブアドレス 4byte 書き込み
GLOBAL const char	cmd_w5[5]  	GLOBAL_VAL("w5\0");	// ランダム読み込み用 サブアドレス 5byte 書き込み
GLOBAL const char	cmd_ww5[5]  GLOBAL_VAL("W5\0");	// ランダム読み込み用 サブアドレス 5byte 書き込み
GLOBAL const char	cmd_w6[6]  	GLOBAL_VAL("w6\0");	// ランダム読み込み用 サブアドレス 6byte 書き込み
GLOBAL const char	cmd_ww6[6]  GLOBAL_VAL("W6\0");	// ランダム読み込み用 サブアドレス 6byte 書き込み
GLOBAL const char	cmd_w7[7]  	GLOBAL_VAL("w7\0");	// ランダム読み込み用 サブアドレス 7byte 書き込み
GLOBAL const char	cmd_ww7[7]  GLOBAL_VAL("W7\0");	// ランダム読み込み用 サブアドレス 7byte 書き込み
GLOBAL const char	cmd_w8[8]  	GLOBAL_VAL("w8\0");	// ランダム読み込み用 サブアドレス 8byte 書き込み
GLOBAL const char	cmd_ww8[8]  GLOBAL_VAL("W8\0");	// ランダム読み込み用 サブアドレス 8byte 書き込み
GLOBAL const char	cmd_r[2] 	GLOBAL_VAL("r\0");	// 読み込みコマンド
GLOBAL const char	cmd_rr[2] 	GLOBAL_VAL("R\0");	// 読み込みコマンド

// 表示文字列のマクロ化

#define _NG_	"\n <NG> \r\n"	//
#define _OK_	"\n <OK> "		//

#define _BYTE_	"%02bx "		//
#define _WORD_	"%04bx "		//
//***************************************************************
// keilでsprintfを使うとき、_BYTE_ が %02x だと、表示がおかしくなる。
// 表示する変数のビット長により 8bitだとb、32bitだとlを付けること。
// 上記だと %02x → %02bx とする。
//***************************************************************

GLOBAL uint8 sample_full	GLOBAL_VAL(0x00);

/* [] END OF FILE */
