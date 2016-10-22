
#include <project.h>
#include <string.h>
#include <stdio.h>
#include <cytypes.h>

#include "prot.h"
#define	 GLOBAL_VALUE_DEFINE    // def.h に対して　main.cは設定。　他のsubファイルは設定しない。
#include "def.h"



//***************************************************************
// ADコンバータ 変換終了割込み
//***************************************************************
//===============================================================
CY_ISR(ISR_ADC_End_of_Conversion)
//===============================================================
{
	
    sample_full = ADC_DelSig_GetResult8();		// 変換結果の取得 ８ｂｉｔ

	sprintf( tmp_chr, _BYTE_ , sample_full );		// 読取りデータの表示
	while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
	USBUART_1_PutString( tmp_chr );

	CyDelay(1000u/*ms*/);
	
	AMuxSeq_Next(); 		// これないとおかしくなるよ


}


//===============================================================
int main()
//===============================================================
{

	uint8 i = 0;
	uint8 j = 0;
	uint8 k = 0;
	uint8 status;
	
	// *** USBUART用 *** //
	char msg[64];	// USBUARTからの受信メッセージバッファ
	int	 rcvLen;	// そのデータ長
	
	// *** コマンド解析用 *** //
	char delimi[] = " \r";  /* 空白+CR */
	char *tok =	"\0";

	// *** コマンド配列の初期化 *** //
	for( i=0; i < CMD_PARA_SIZE; i++ ) {
		bCmdParam[i]='\0';
	}

	// グローバル割込みをイネーブル
	CyGlobalIntEnable;


	// *** USBモジュールの初期化 *** //
    USBUART_1_Start(0, USBUART_1_3V_OPERATION);		//PSOCを、3.3V電源で使用する場合
	while( !USBUART_1_GetConfiguration() ){};		//初期化終了まで待機
	USBUART_1_CDC_Init();							//USBFSのCDCコンポーネントを初期化

	// *** 各モジュールの初期化 *** //
	I2C_M_Start();
	AMuxSeq_Init();
	ADC_DelSig_Start();
	


	// *** 起動時のウエイト *** //
	CyDelay(1000u/*ms*/);

	isr_ADC_DelSig_StartEx(ISR_ADC_End_of_Conversion);		//AD変換完了割込

	// *** ADC変換開始 *** //
//    ADC_DelSig_StartConvert();
	

	// *** デフォルト pullup無し *** //
	PU_SDA_Write(1u);
	PU_SCL_Write(1u);
	

	while(1)
	{
		if( USBUART_1_DataIsReady() )				//PCからのデータ受信待ち
		{
			rcvLen = USBUART_1_GetCount();		//受信したデータ長の取得
			USBUART_1_GetData( msg, rcvLen );		//受信データを取得

			for( k=0; k<rcvLen; k++ ) {
			
				cha[0] = msg[k];				// 受信データの文字列を作る
				strcat( cmd_buffer, cha );		//最初の一文字
			
				// 受信文字がCRまたはLFか？
				if ( (msg[k] == 0x0d) || (msg[k] == 0x0a) )
				// YES
				{	
					tok = strtok(cmd_buffer, delimi);
					
					while( tok != NULL ){
						bCmdParam[i++] = tok;
						tok = strtok( NULL, delimi );  // 2回目以降 区切り文字がNULLに置き換えられるので取り除く
					}
					
					// cmd_w	書き込みコマンド
					/*
						bCmdParam   0 1  2 3 4 5 6 7
								i	1 2  3 4 5 6 7 8
								j          0 1 
					1.バイト書き込み	s A0 w a a b p
					2.ページ書き込み	s A0 w a a d d d p
					*/

					if (   ( !(strcmp(cmd_w, bCmdParam[2])) || !(strcmp(cmd_ww, bCmdParam[2])) ) 
						&& ( !(strcmp(cmd_s, bCmdParam[0])) || !(strcmp(cmd_ss, bCmdParam[0])) ) 
						&& ( !(strcmp(cmd_p, bCmdParam[i-1])) || !(strcmp(cmd_pp, bCmdParam[i-1])) ) )
					{
						dev_addr = ToDec(bCmdParam[1])>>1;		// dev_addrは、8bit 偶数とする。
						
						for( j=0; j<(i-4); j++ ) {							// to int が必要なパラメータは、4少ない
							i2cBufferWrite[j] = ToDec(bCmdParam[j+3]);	// 16進文字列を10進数に変換
						}
						
						I2C_M_MasterClearStatus();						// Clear any previous status  
						I2C_M_MasterWriteBuf( dev_addr, (uint8 *)i2cBufferWrite, i-4, I2C_M_MODE_COMPLETE_XFER );	// 書き込み 

						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_WR_CMPLT ) ) break; 		// 書き込み完了を待つ
						}
						
						if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_ERR_XFER ) ) {				// エラーの有無
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _NG_ );
						} else {
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _OK_ );
						}
					} 
					
					// cmd_r	サブアドレス指定なしの読み込みコマンド
					/*	
						bCmdParam	0 1  2 3 4 
								i	1 2  3 4 5
						2.読み込み 	 s A0 r n p	
					*/
					
					else if (  ( !(strcmp(cmd_r, bCmdParam[2])) || !(strcmp(cmd_rr, bCmdParam[2])) ) 
							&& ( !(strcmp(cmd_s, bCmdParam[0])) || !(strcmp(cmd_ss, bCmdParam[0])) ) 
							&& ( !(strcmp(cmd_p, bCmdParam[i-1])) || !(strcmp(cmd_pp, bCmdParam[i-1])) )
							&& (i == 5) )
					{	
						dev_addr = ToDec( bCmdParam[1] )>>1;		
						buf_cnt  = ToDec( bCmdParam[3] );		
						
						I2C_M_MasterClearStatus(); 						// Clear any previous status  
						I2C_M_MasterReadBuf( dev_addr, (uint8 *)i2cBufferRead, buf_cnt, I2C_M_MODE_COMPLETE_XFER);	// 読み込み 
						
						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_RD_CMPLT ) ) break; 		// 読み込み完了を待つ
						}
						
						if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_ERR_XFER ) ) {				// エラーの有無
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _NG_ );
						} else {
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _OK_ );
						
							for( j=0; j<buf_cnt;j++ ) {
								sprintf( tmp_chr, _BYTE_ , i2cBufferRead[j] );		// 読取りデータの表示
								while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
								USBUART_1_PutString( tmp_chr );
							}
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutCRLF();	// 改行
						}
					} 

					
					// cmd_w1 cmd_r	アドレス指定ありの読み込みコマンド
					/*	
						bCmdParam	0 1  2  3  4 5 6  
								i	1 2  3  4  5 6 7
				ランダム読み込み	   s A0 w1 ad r n p	
					*/

					else if (  ( !(strcmp(cmd_w1, bCmdParam[2])) || !(strcmp(cmd_ww1, bCmdParam[2])) ) 
							&& ( !(strcmp(cmd_s,  bCmdParam[0])) || !(strcmp(cmd_ss,  bCmdParam[0])) ) 
							&& ( !(strcmp(cmd_r,  bCmdParam[4])) || !(strcmp(cmd_rr,  bCmdParam[4])) ) 
							&& ( !(strcmp(cmd_p,  bCmdParam[6])) || !(strcmp(cmd_pp,  bCmdParam[6])) )
							&& (i == 7) )
					{	
						dev_addr = ToDec( bCmdParam[1] )>>1;		
						buf_cnt  = ToDec( bCmdParam[5] );	
						
						i2cBufferWrite[0] = ToDec( bCmdParam[3] );	// 指定アドレス、16進文字列を10進数に変換
						
						I2C_M_MasterClearStatus(); 					// Clear any previous status 
						I2C_M_MasterWriteBuf( dev_addr, (uint8 *)i2cBufferWrite, 1, I2C_M_MODE_NO_STOP );	// サブアドレス1byte書き込み 
						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_WR_CMPLT ) ) break; 		// 書き込み完了を待つ
						}
						
						I2C_M_MasterReadBuf( dev_addr, (uint8 *)i2cBufferRead, buf_cnt, I2C_M_MODE_REPEAT_START );	// 読み込み 
						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_RD_CMPLT ) ) break; 		// 読み込み完了を待つ
						}
						
						if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_ERR_XFER ) ) {				// エラーの有無
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _NG_ );
						} else {
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _OK_ );
						
							for( j=0; j<buf_cnt; j++ ) {
								sprintf( tmp_chr, _BYTE_ , i2cBufferRead[j] );		// 読取りデータの表示
								while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
								USBUART_1_PutString( tmp_chr );
							}
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutCRLF();	// 改行
							}
					}
					
					// cmd_w2 cmd_r	アドレス指定ありの読み込みコマンド
					/*	
						bCmdParam	0 1  2  3  4 5 6 7  
								i	1 2  3  4  5 6 7 8 
				ランダム読み込み	   s A0 w2 ad ad r n p	
					*/
					
					else if (  ( !(strcmp(cmd_w2, bCmdParam[2])) || !(strcmp(cmd_ww2, bCmdParam[2])) ) 
							&& ( !(strcmp(cmd_s,  bCmdParam[0])) || !(strcmp(cmd_ss,  bCmdParam[0])) ) 
							&& ( !(strcmp(cmd_r,  bCmdParam[5])) || !(strcmp(cmd_rr,  bCmdParam[5])) ) 
							&& ( !(strcmp(cmd_p,  bCmdParam[7])) || !(strcmp(cmd_pp,  bCmdParam[7])) )
							&& (i == 8) )
					{	
						
						dev_addr = ToDec( bCmdParam[1] )>>1;		
						buf_cnt  = ToDec( bCmdParam[6] );	
						
						i2cBufferWrite[0] = ToDec( bCmdParam[3] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[1] = ToDec( bCmdParam[4] );	// 指定アドレス、16進文字列を10進数に変換

						
						I2C_M_MasterClearStatus(); 					// Clear any previous status 
						I2C_M_MasterWriteBuf( dev_addr, (uint8 *)i2cBufferWrite, 2, I2C_M_MODE_NO_STOP );	// サブアドレス2byte書き込み  
						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_WR_CMPLT ) ) break; 		// 書き込み完了を待つ
						}
						
						I2C_M_MasterReadBuf( dev_addr, (uint8 *) i2cBufferRead, buf_cnt, I2C_M_MODE_REPEAT_START );	// 読み込み  
						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_RD_CMPLT ) ) break; 		// 読み込み完了を待つ
						}
						
						if( 0u != (I2C_M_MasterStatus() & I2C_M_MSTAT_ERR_XFER ) ) {				// エラーの有無
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _NG_ );
						} else {
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _OK_ );
						
							for( j=0; j<buf_cnt; j++ ) {
								sprintf( tmp_chr, _BYTE_ , i2cBufferRead[j] );	// 読取りデータの表示
								while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
								USBUART_1_PutString( tmp_chr );
							}
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutCRLF();	// 改行
						}
					}
					
					// cmd_w3 cmd_r	アドレス指定ありの読み込みコマンド
					/*	
						bCmdParam	0 1  2  3  4  5  6 7 8  
								i	1 2  3  4  5  6  7 8 9
				ランダム読み込み	   s A0 w3 ad ad ad  r n p	
					*/
					
					else if (  ( !(strcmp(cmd_w3, bCmdParam[2])) || !(strcmp(cmd_ww3, bCmdParam[2])) ) 
							&& ( !(strcmp(cmd_s,  bCmdParam[0])) || !(strcmp(cmd_ss,  bCmdParam[0])) ) 
							&& ( !(strcmp(cmd_r,  bCmdParam[6])) || !(strcmp(cmd_rr,  bCmdParam[6])) ) 
							&& ( !(strcmp(cmd_p,  bCmdParam[8])) || !(strcmp(cmd_pp,  bCmdParam[8])) )
							&& (i == 9) )
					{	
						
						dev_addr = ToDec( bCmdParam[1] )>>1;		
						buf_cnt  = ToDec( bCmdParam[7] );	
						
						i2cBufferWrite[0] = ToDec( bCmdParam[3] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[1] = ToDec( bCmdParam[4] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[2] = ToDec( bCmdParam[5] );	// 指定アドレス、16進文字列を10進数に変換

						
						I2C_M_MasterClearStatus(); 					// Clear any previous status 
						I2C_M_MasterWriteBuf( dev_addr, (uint8 *)i2cBufferWrite, 3, I2C_M_MODE_NO_STOP );	// サブアドレス3byte書き込み  
						for(;;) { 
							if( 0u != (I2C_M_MasterStatus() & I2C_M_MSTAT_WR_CMPLT ) ) break; 		// 書き込み完了を待つ
						}
						
						I2C_M_MasterReadBuf( dev_addr, (uint8 *)i2cBufferRead, buf_cnt, I2C_M_MODE_REPEAT_START );	// 読み込み  
						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_RD_CMPLT ) ) break; 		// 読み込み完了を待つ
						}
						
						if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_ERR_XFER ) ) {				// エラーの有無
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _NG_ );
						} else {
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _OK_ );
						
							for( j=0; j<buf_cnt; j++ ) {
								sprintf( tmp_chr, _BYTE_ , i2cBufferRead[j] );	// 読取りデータの表示
								while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
								USBUART_1_PutString( tmp_chr );
							}
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutCRLF();	// 改行
						}
					}
					
					// cmd_w4 cmd_r	アドレス指定ありの読み込みコマンド
					/*	
						bCmdParam	0 1  2  3  4  5  6 7 8 9  
								i	1 2  3  4  5  6  7 8 9 10
				ランダム読み込み	   s A0 w4 a1 a2 a3 a4 r n p	
					*/
					
					else if (  ( !(strcmp(cmd_w4, bCmdParam[2])) || !(strcmp(cmd_ww4, bCmdParam[2])) ) 
							&& ( !(strcmp(cmd_s,  bCmdParam[0])) || !(strcmp(cmd_ss,  bCmdParam[0])) ) 
							&& ( !(strcmp(cmd_r,  bCmdParam[7])) || !(strcmp(cmd_rr,  bCmdParam[7])) ) 
							&& ( !(strcmp(cmd_p,  bCmdParam[9])) || !(strcmp(cmd_pp,  bCmdParam[9])) )
							&& (i == 10) )
					{	
						
						dev_addr = ToDec( bCmdParam[1] )>>1;		
						buf_cnt  = ToDec( bCmdParam[8] );	
						
						i2cBufferWrite[0] = ToDec( bCmdParam[3] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[1] = ToDec( bCmdParam[4] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[2] = ToDec( bCmdParam[5] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[3] = ToDec( bCmdParam[6] );	// 指定アドレス、16進文字列を10進数に変換

						
						I2C_M_MasterClearStatus(); 					// Clear any previous status 
						I2C_M_MasterWriteBuf( dev_addr, (uint8 *)i2cBufferWrite, 4, I2C_M_MODE_NO_STOP );	// サブアドレス3byte書き込み  
						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_WR_CMPLT ) ) break; 		// 書き込み完了を待つ
						}
						
						I2C_M_MasterReadBuf( dev_addr, (uint8 *)i2cBufferRead, buf_cnt, I2C_M_MODE_REPEAT_START );	// 読み込み  
						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_RD_CMPLT ) ) break; 		// 読み込み完了を待つ
						}
						
						if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_ERR_XFER ) ) {				// エラーの有無
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _NG_ );
						} else {
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _OK_ );
						
							for( j=0; j<buf_cnt; j++ ) {
								sprintf( tmp_chr, _BYTE_ , i2cBufferRead[j] );	// 読取りデータの表示
								while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
								USBUART_1_PutString( tmp_chr );
							}
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutCRLF();	// 改行
						}
					}
					
					// cmd_w5 cmd_r	アドレス指定ありの読み込みコマンド
					/*	
						bCmdParam	0 1  2  3  4  5  6  7 8  9 10  
								i	1 2  3  4  5  6  7  8 9 10 11
				ランダム読み込み	   s A0 w5 a1 a2 a3 a4 a5 r n  p	
					*/
					
					else if (  ( !(strcmp(cmd_w5, bCmdParam[2]))  || !(strcmp(cmd_ww5, bCmdParam[2])) ) 
							&& ( !(strcmp(cmd_s,  bCmdParam[0]))  || !(strcmp(cmd_ss,  bCmdParam[0])) ) 
							&& ( !(strcmp(cmd_r,  bCmdParam[8]))  || !(strcmp(cmd_rr,  bCmdParam[8])) ) 
							&& ( !(strcmp(cmd_p,  bCmdParam[10])) || !(strcmp(cmd_pp,  bCmdParam[10])) )
							&& (i == 11) )
					{	
						
						dev_addr = ToDec( bCmdParam[1] )>>1;		
						buf_cnt  = ToDec( bCmdParam[9] );	
						
						i2cBufferWrite[0] = ToDec( bCmdParam[3] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[1] = ToDec( bCmdParam[4] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[2] = ToDec( bCmdParam[5] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[3] = ToDec( bCmdParam[6] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[4] = ToDec( bCmdParam[7] );	// 指定アドレス、16進文字列を10進数に変換

						
						I2C_M_MasterClearStatus(); 					// Clear any previous status 
						I2C_M_MasterWriteBuf( dev_addr, (uint8 *)i2cBufferWrite, 5, I2C_M_MODE_NO_STOP );	// サブアドレス3byte書き込み  
						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_WR_CMPLT ) ) break; 		// 書き込み完了を待つ
						}
						
						I2C_M_MasterReadBuf( dev_addr, (uint8 *)i2cBufferRead, buf_cnt, I2C_M_MODE_REPEAT_START );	// 読み込み  
						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_RD_CMPLT ) ) break; 		// 読み込み完了を待つ
						}
						
						if( 0u != (I2C_M_MasterStatus() & I2C_M_MSTAT_ERR_XFER ) ) {				// エラーの有無
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _NG_ );
						} else {
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _OK_ );
						
							for( j=0; j<buf_cnt; j++ ) {
								sprintf( tmp_chr, _BYTE_ , i2cBufferRead[j] );	// 読取りデータの表示
								while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
								USBUART_1_PutString( tmp_chr );
							}
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutCRLF();	// 改行
						}
					}
					
					// cmd_w6 cmd_r	アドレス指定ありの読み込みコマンド
					/*	
						bCmdParam	0 1  2  3  4  5  6  7  8  9 10 11  
								i	1 2  3  4  5  6  7  8  9 10 11 12
				ランダム読み込み	   s A0 w6 a1 a2 a3 a4 a5 a6 r  n  p	
					*/
					
					else if (  ( !(strcmp(cmd_w6, bCmdParam[2]))  || !(strcmp(cmd_ww6, bCmdParam[2])) ) 
							&& ( !(strcmp(cmd_s,  bCmdParam[0]))  || !(strcmp(cmd_ss,  bCmdParam[0])) ) 
							&& ( !(strcmp(cmd_r,  bCmdParam[9]))  || !(strcmp(cmd_rr,  bCmdParam[9])) ) 
							&& ( !(strcmp(cmd_p,  bCmdParam[11])) || !(strcmp(cmd_pp,  bCmdParam[11])) )
							&& (i == 12) )
					{	
						
						dev_addr = ToDec( bCmdParam[1] )>>1;		
						buf_cnt  = ToDec( bCmdParam[10] );	
						
						i2cBufferWrite[0] = ToDec( bCmdParam[3] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[1] = ToDec( bCmdParam[4] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[2] = ToDec( bCmdParam[5] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[3] = ToDec( bCmdParam[6] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[4] = ToDec( bCmdParam[7] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[5] = ToDec( bCmdParam[8] );	// 指定アドレス、16進文字列を10進数に変換

						
						I2C_M_MasterClearStatus(); 					// Clear any previous status 
						I2C_M_MasterWriteBuf( dev_addr, (uint8 *)i2cBufferWrite, 6, I2C_M_MODE_NO_STOP );	// サブアドレス3byte書き込み  
						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_WR_CMPLT ) ) break; 		// 書き込み完了を待つ
						}
						
						I2C_M_MasterReadBuf( dev_addr, (uint8 *)i2cBufferRead, buf_cnt, I2C_M_MODE_REPEAT_START );	// 読み込み  
						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_RD_CMPLT ) ) break; 		// 読み込み完了を待つ
						}
						
						if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_ERR_XFER ) ) {				// エラーの有無
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _NG_ );
						} else {
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _OK_ );
						
							for( j=0; j<buf_cnt; j++ ) {
								sprintf( tmp_chr, _BYTE_ , i2cBufferRead[j] );	// 読取りデータの表示
								while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
								USBUART_1_PutString( tmp_chr );
							}
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutCRLF();	// 改行
						}
					}
					
					// cmd_w7 cmd_r	アドレス指定ありの読み込みコマンド
					/*	
						bCmdParam	0 1  2  3  4  5  6  7  8  9  10 11 12   
								i	1 2  3  4  5  6  7  8  9 10  11 12 13
				ランダム読み込み	   s A0 w7 a1 a2 a3 a4 a5 a6 a7  r  n  p	
					*/
					
					else if (  ( !(strcmp(cmd_w7, bCmdParam[2]))  || !(strcmp(cmd_ww7, bCmdParam[2])) ) 
							&& ( !(strcmp(cmd_s,  bCmdParam[0]))  || !(strcmp(cmd_ss,  bCmdParam[0])) ) 
							&& ( !(strcmp(cmd_r,  bCmdParam[10])) || !(strcmp(cmd_rr,  bCmdParam[10])) ) 
							&& ( !(strcmp(cmd_p,  bCmdParam[12])) || !(strcmp(cmd_pp,  bCmdParam[12])) )
							&& (i == 13) )
					{	
						
						dev_addr = ToDec( bCmdParam[1] )>>1;		
						buf_cnt  = ToDec( bCmdParam[11] );	
						
						i2cBufferWrite[0] = ToDec( bCmdParam[3] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[1] = ToDec( bCmdParam[4] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[2] = ToDec( bCmdParam[5] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[3] = ToDec( bCmdParam[6] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[4] = ToDec( bCmdParam[7] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[5] = ToDec( bCmdParam[8] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[6] = ToDec( bCmdParam[9] );	// 指定アドレス、16進文字列を10進数に変換

						
						I2C_M_MasterClearStatus(); 					// Clear any previous status 
						I2C_M_MasterWriteBuf( dev_addr, (uint8 *)i2cBufferWrite, 7, I2C_M_MODE_NO_STOP );	// サブアドレス3byte書き込み  
						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_WR_CMPLT ) ) break; 		// 書き込み完了を待つ
						}
						
						I2C_M_MasterReadBuf( dev_addr, (uint8 *)i2cBufferRead, buf_cnt, I2C_M_MODE_REPEAT_START );	// 読み込み  
						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_RD_CMPLT ) ) break; 		// 読み込み完了を待つ
						}
						
						if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_ERR_XFER ) ) {				// エラーの有無
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _NG_ );
						} else {
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _OK_ );
						
							for( j=0; j<buf_cnt; j++ ) {
								sprintf( tmp_chr, _BYTE_ , i2cBufferRead[j] );	// 読取りデータの表示
								while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
								USBUART_1_PutString( tmp_chr );
							}
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutCRLF();	// 改行
						}
					}
					
					// cmd_w8 cmd_r	アドレス指定ありの読み込みコマンド
					/*	
						bCmdParam	0 1  2  3  4  5  6  7  8  9 10 11 12 13  
								i	1 2  3  4  5  6  7  8  9 10 11 12 13 14
				ランダム読み込み	   s A0 w8 a1 a2 a3 a4 a5 a6 a7 a8 r  n  p	
					*/
					
					else if (  ( !(strcmp(cmd_w8, bCmdParam[2]))  || !(strcmp(cmd_ww8, bCmdParam[2])) ) 
							&& ( !(strcmp(cmd_s,  bCmdParam[0]))  || !(strcmp(cmd_ss,  bCmdParam[0])) ) 
							&& ( !(strcmp(cmd_r,  bCmdParam[11])) || !(strcmp(cmd_rr,  bCmdParam[11])) ) 
							&& ( !(strcmp(cmd_p,  bCmdParam[13])) || !(strcmp(cmd_pp,  bCmdParam[13])) )
							&& (i == 14) )
					{	
						
						dev_addr = ToDec( bCmdParam[1] )>>1;		
						buf_cnt  = ToDec( bCmdParam[12] );	
						
						i2cBufferWrite[0] = ToDec( bCmdParam[3] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[1] = ToDec( bCmdParam[4] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[2] = ToDec( bCmdParam[5] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[3] = ToDec( bCmdParam[6] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[4] = ToDec( bCmdParam[7] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[5] = ToDec( bCmdParam[8] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[6] = ToDec( bCmdParam[9] );	// 指定アドレス、16進文字列を10進数に変換
						i2cBufferWrite[7] = ToDec( bCmdParam[10] );	// 指定アドレス、16進文字列を10進数に変換

						
						I2C_M_MasterClearStatus(); 					// Clear any previous status 
						I2C_M_MasterWriteBuf( dev_addr, (uint8 *)i2cBufferWrite, 8, I2C_M_MODE_NO_STOP );	// サブアドレス3byte書き込み  
						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_WR_CMPLT ) ) break; 		// 書き込み完了を待つ
						}
						
						I2C_M_MasterReadBuf( dev_addr, (uint8 *)i2cBufferRead, buf_cnt, I2C_M_MODE_REPEAT_START );	// 読み込み  
						for(;;) { 
							if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_RD_CMPLT ) ) break; 		// 読み込み完了を待つ
						}
						
						if( 0u != ( I2C_M_MasterStatus() & I2C_M_MSTAT_ERR_XFER ) ) {				// エラーの有無
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _NG_ );
							
						} else {
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutString( _OK_ );
						
							for( j=0; j<buf_cnt; j++ ) {
								sprintf( tmp_chr, _BYTE_ , i2cBufferRead[j] );	// 読取りデータの表示
								while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
								USBUART_1_PutString( tmp_chr );
							}
							
							while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
							USBUART_1_PutCRLF();	// 改行
						}
					}
					
					else {
	        		        while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機 
							USBUART_1_PutString( "\n  Command Error\r\n" );
					}
					
					memset( cmd_buffer, 0, 420 );
					i = 0;
					
	                while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機 
					USBUART_1_PutString( "\r\n>" );
					
				}
			}
		}
		
		/*
	    ADC_DelSig_IsEndConversion(ADC_DelSig_WAIT_FOR_RESULT);
	     Since the ADC conversion is complete, stop the conversion
	    sample_full = ADC_DelSig_GetResult16();

		sprintf( tmp_chr, _BYTE_ , sample_full );		// 読取りデータの表示
		while( USBUART_1_CDCIsReady() == 0u );    // 送信可能まで待機
		USBUART_1_PutString( tmp_chr );
		
		AMuxSeq_Next(); 

		CyDelay(1000u);
		*/
	}
}
				
	
				

  /* 16 進文字列を 10 進数に変換する */
unsigned long ToDec(const char str[ ])
{
    short i = 0;        /* 配列の添字として使用 */
    short n;
    unsigned long x = 0;
    char c;

    while ( str[i] != '\0' ) {        /* 文字列の末尾でなければ */

            /* '0' から '9' の文字なら */
        if  ( '0' <= str[i]  &&  str[i] <= '9' )
            n = str[i] - '0';        /* 数字に変換 */

            /* 'a' から 'f' の文字なら */
        else if ( 'a' <= (c = tolower(str[i])) && c <= 'f' )
            n = c - 'a' + 10;        /* 数字に変換 */

        else {        /* それ以外の文字なら */
			
			sprintf(tmp_chr, "Error  %3s", str[i]);
			while( USBUART_1_CDCIsReady() == 0u);    // 送信可能まで待機
			USBUART_1_PutString(tmp_chr);
            break;        /* プログラムを終了させる */
        }
        i++;        /* 次の文字を指す */

        x = x *16 + n;    /* 桁上がり */
    }
    return (x);
}

/* [] END OF FILE */
