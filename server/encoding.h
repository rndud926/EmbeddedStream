#ifndef __ENCODING_H__
#define __ENCODING_H__

#include "SsbSipMfcApi.h"
#include "mfc_interface.h"
#include "mfc_errorno.h"


int width = 800;
int height = 480;

void yuy2tonv12(char *yuy2, char *nv12l, int y_size);
void nv12ltonv12t(char *nv12l, char *nv12t, int y_size, int aligned_y_size);
void linear_to_tile_4x2(char *p_linear_addr, char *p_tiled_addr, int x_size, int y_size);
int tile_4x2_read(int x_size, int y_size, int x_pos, int y_pos);
void copy16(char *p_linear_addr, char *p_tiled_addr, int mm, int nn);
void init_encoding_set(SSBSIP_MFC_ENC_H264_PARAM *encoding_set);



void nv12ltonv12t(char *nv12l, char *nv12t, int y_size, int aligned_y_size)
{
	//printf("start nv12ltonv12t\n");
	char *tile_y_buf_addr;
	char *tile_c_buf_addr;
	char *linear_y_buf_addr;
	char *linear_c_buf_addr;

	tile_y_buf_addr = nv12t;
	tile_c_buf_addr = nv12t + aligned_y_size;
	linear_y_buf_addr = nv12l;
	linear_c_buf_addr = nv12l + y_size;

	linear_to_tile_4x2(linear_y_buf_addr, tile_y_buf_addr, width, height); //y���� nv12l ->nv12t
	linear_to_tile_4x2(linear_c_buf_addr, tile_c_buf_addr, width, height >> 1); //c���� nv12l -> nv12t 
	
	//printf("end nv12ltonv12t\n");
}

void linear_to_tile_4x2(char *p_linear_addr, char *p_tiled_addr, int x_size, int y_size)
{
	//���⼭ �״±��� 
	//printf("start linear_to_tile_4x2\n");
	int i, j, k, nn, mm, index, trans_addr;

	for(i = 0; i < y_size; i = i + 16)
	{
		for(j = 0; j < x_size; j = j + 16)
		{
			//printf(" (1 %d %d ) \n", i,j);
			trans_addr = tile_4x2_read(x_size, y_size, j, i);			
			//printf(" (2 %d %d ) \n", i,j);
			index = i * x_size + j;
			//printf("index\n");

			k = 0;		nn = trans_addr + (k << 6);		mm = index;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);
			//printf("0\n");
			
			k = 1;		nn = trans_addr + (k << 6);		mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);			
			//printf("1\n");
			
			k = 2;		nn = trans_addr + (k << 6);		mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);
			//printf("2\n");
			
			k = 3;		nn = trans_addr + (k << 6);		mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);
			//printf("3\n");
			
			k = 4;		nn = trans_addr + (k << 6);		mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);
			//printf("4\n");
			
			k = 5;		nn = trans_addr + (k << 6);		mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);			
			//printf("5\n");
			
			k = 6;		nn = trans_addr + (k << 6);		mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);
			//printf("6\n");
			
			k = 7;		nn = trans_addr + (k << 6);		mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);
			//printf("7\n");
			
			k = 8;		nn = trans_addr + (k << 6);		mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);
			//printf("8\n");
			
			k = 9;		nn = trans_addr + (k << 6);		mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);			
			//printf("9\n");
			
			k = 10;		nn = trans_addr + (k << 6);		mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);
			//printf("10\n");
			
			k = 11;		nn = trans_addr + (k << 6);		mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);
			//printf("11\n");
			
			k = 12;		nn = trans_addr + (k << 6);		mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);
			//printf("12\n");
			
			k = 13;		nn = trans_addr + (k << 6);		mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);			
			//printf("13\n");
			
			k = 14;		nn = trans_addr + (k << 6);		mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);
			//printf("14\n");
			
			k = 15;		nn = trans_addr + (k << 6);		mm += x_size;
			copy16(p_linear_addr, p_tiled_addr, mm, nn);
			//printf("15\n");
			
		}
	}
	//printf("end linear_to_tile_4x2\n");
}


int tile_4x2_read(int x_size, int y_size, int x_pos, int y_pos)
{
	//printf("start linear_to_tile_4x2\n");

	int pixel_x_m1, pixel_y_m1;
	int roundup_x, roundup_y;
	int linear_addr0, linear_addr1, bank_addr ;
	int x_addr;
	int trans_addr;

	pixel_x_m1 = x_size - 1;
	pixel_y_m1 = y_size - 1;

	roundup_x = ((pixel_x_m1 >> 7) + 1);
	roundup_y = ((pixel_x_m1 >> 6) + 1);

	x_addr = (x_pos >> 2);

	if ((y_size <= y_pos + 32) && (y_pos < y_size) && (((pixel_y_m1 >> 5) & 0x1) == 0) && (((y_pos >> 5) & 0x1) == 0)){
		linear_addr0 = (((y_pos & 0x1f) <<4) | (x_addr & 0xf));
		linear_addr1 = (((y_pos >> 6) & 0xff) * roundup_x + ((x_addr >> 6) & 0x3f));

		if(((x_addr >> 5) & 0x1) == ((y_pos >> 5) & 0x1))
			bank_addr = ((x_addr >> 4) & 0x1);
		else
			bank_addr = 0x2 | ((x_addr >> 4) & 0x1);
	}
	else{
		linear_addr0 = (((y_pos & 0x1f) << 4) | (x_addr & 0xf));
		linear_addr1 = (((y_pos >> 6) & 0xff) * roundup_x + ((x_addr >> 5) & 0x7f));

		if(((x_addr >> 5) & 0x1) == ((y_pos >> 5) & 0x1))
			bank_addr = ((x_addr >> 4) & 0x1);
		else
			bank_addr = 0x2 | ((x_addr >> 4) & 0x1);
	}

	linear_addr0 = linear_addr0 << 2;
	trans_addr = (linear_addr1 << 13) | (bank_addr << 11) | linear_addr0;

	//printf("end linear_to_tile_4x2\n");

	return trans_addr;
}


void copy16(char *p_linear_addr, char *p_tiled_addr, int mm, int nn)
{
	//printf("start copy16\n");
	p_tiled_addr[nn] = p_linear_addr[mm];
	p_tiled_addr[nn + 1] = p_linear_addr[mm + 1];
	p_tiled_addr[nn + 2] = p_linear_addr[mm + 2];
	p_tiled_addr[nn + 3] = p_linear_addr[mm + 3];
	p_tiled_addr[nn + 4] = p_linear_addr[mm + 4];
	p_tiled_addr[nn + 5] = p_linear_addr[mm + 5];
	p_tiled_addr[nn + 6] = p_linear_addr[mm + 6];
	p_tiled_addr[nn + 7] = p_linear_addr[mm + 7];
	p_tiled_addr[nn + 8] = p_linear_addr[mm + 8];
	p_tiled_addr[nn + 9] = p_linear_addr[mm + 9];
	p_tiled_addr[nn + 10] = p_linear_addr[mm + 10];
	p_tiled_addr[nn + 11] = p_linear_addr[mm + 11];
	p_tiled_addr[nn + 12] = p_linear_addr[mm + 12];
	p_tiled_addr[nn + 13] = p_linear_addr[mm + 13];
	p_tiled_addr[nn + 14] = p_linear_addr[mm + 14];
	p_tiled_addr[nn + 15] = p_linear_addr[mm + 15];
	//printf("end copy16\n");
}


void init_encoding_set(SSBSIP_MFC_ENC_H264_PARAM *encoding_set)
{
	encoding_set->codecType = H264_ENC;
	encoding_set->SourceWidth = width;
	encoding_set->SourceHeight = height;
	encoding_set->IDRPeriod = 10;
	encoding_set->SliceMode = 0;
	encoding_set->RandomIntraMBRefresh = 0;
	encoding_set->EnableFRMRateControl = 0;
	encoding_set->Bitrate = 1024;
	encoding_set->FrameQp = 30;
	encoding_set->FrameQp_P = 30;
	encoding_set->FrameQp_B = 30;
	encoding_set->QSCodeMax = 51;
	encoding_set->QSCodeMin = 1;
	encoding_set->CBRPeriodRf = 0;
	encoding_set->PadControlOn = 0;
	encoding_set->LumaPadVal = 0;
	encoding_set->CbPadVal = 0;
	encoding_set->CrPadVal = 0;
	encoding_set->ProfileIDC = 2;
	encoding_set->LevelIDC = 40;
	encoding_set->FrameRate = 15;
	encoding_set->SliceArgument = 0;
	encoding_set->NumberBFrames = 0;
	encoding_set->NumberReferenceFrames = 1;
	encoding_set->NumberRefForPframes = 1;
	encoding_set->LoopFilterDisable = 0;
	encoding_set->LoopFilterAlphaC0Offset = 0;
	encoding_set->LoopFilterBetaOffset = 0;
	encoding_set->SymbolMode = 0;
	encoding_set->PictureInterlace = 0;
	encoding_set->Transform8x8Mode = 0;
	encoding_set->EnableMBRateControl = 0;
	encoding_set->DarkDisable = 0;
	encoding_set->SmoothDisable = 0;
	encoding_set->StaticDisable = 0;
	encoding_set->ActivityDisable = 0;
}


void yuy2tonv12(char *yuy2, char *nv12l, int y_size)
{
	char y0, u0, y1, v0, y2, u1, y3, v1;
	char *Y_1, *Y_2, *U, *V; //nv12l�� ������ ������ ����
	char *YUY2_1, *YUY2_2; //yuy2���� ������ ������ ����
	int i, j;

	U = nv12l + y_size;
	V = U + 1;
	
	for(i = 0; i < height; i = i+2){
		YUY2_1 = yuy2 + width * 2 * i;
		YUY2_2 = YUY2_1 + width * 2;
		Y_1 = nv12l + width * i;
		Y_2 = Y_1 + width;
		for(j = 0; j < width * 2; j = j+4){
			y0 = YUY2_1[0];
			u0 = YUY2_1[1];
			y1 = YUY2_1[2];
			v0 = YUY2_1[3];
			y2 = YUY2_2[0];
			u1 = YUY2_2[1];
			y3 = YUY2_2[2];
			v1 = YUY2_2[3];

			*(Y_1 + 0) = y0;
			*(Y_1 + 1) = y1;
			*(Y_2 + 0) = y2;
			*(Y_2 + 1) = y3;
			*U = (u0 + u1) / 2;
			*V = (v0 + v1) / 2;

			YUY2_1 += 4;
			YUY2_2 += 4;
			Y_1 += 2;
			Y_2 += 2;
			U += 2;
			V += 2;
		}
	}
}


#endif
