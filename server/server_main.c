#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>

#include "video.h"
#include "SsbSipMfcApi.h"
#include "mfc_interface.h"
#include "mfc_errorno.h"
#include "server.h"
#include "encoding.h"
	
int main(int argc, char **argv)
{
	int port = atoi(argv[1]);
	//time & file
	time_t curr;
	struct tm *today;
	char file_name[256]={0,};

	time(&curr);
	today = localtime(&curr);
	
	char* v4l2_data;
	char* nv12l, *nv12t;
	int y_size , c_size;
	int aligned_y_size, aligend_c_size; //padding�� ���Ե� ���� ??
	_MFCLIB *encoding = NULL;
	_MFCLIB *decoding = NULL;
	int mfc_ret;

	y_size = width * height; //y���� ũ��
	c_size = (width * height)/2; //u,v, ���� ũ��
	
	aligned_y_size =ALIGN_TO_128B(width) * ALIGN_TO_32B(height); //�� �𸣰���. aligned�ǹ�. 
	aligend_c_size =ALIGN_TO_128B(width) * ALIGN_TO_32B(height/2);


	printf("y_size : %d \nc_size : %d \naligned_y_size : %d \naligend_c_size : %d \n"
		, y_size,  c_size, aligned_y_size,aligend_c_size);
	printf("aligned_y_size : ALIGN_TO_128B(width) * ALIGN_TO_32B(height) %d %d \n", ALIGN_TO_128B(width) , ALIGN_TO_32B(height));
	printf("aligned_c_size : ALIGN_TO_128B(width) * ALIGN_TO_32B(height) %d %d \n", ALIGN_TO_128B(width) , ALIGN_TO_32B(height/2));

	//encoding
	SSBSIP_MFC_ENC_H264_PARAM encoding_set; //H.264Ÿ��
	SSBSIP_MFC_ENC_INPUT_INFO input_info; //yuv input address , size
	SSBSIP_MFC_ENC_OUTPUT_INFO output_info; //yuv output address , size 
	
	//v4l2 start
	v4l2_init();
	v4l2_start_capturing();
	
	nv12l = (unsigned char *)malloc(y_size + c_size);
	nv12t = (unsigned char *)malloc((aligned_y_size + aligend_c_size)); 

	encoding = SsbSipMfcEncOpen(); //SsbSipMfcEncOpen -> this function is to create the MFC encode instance.
	if(encoding == NULL)
	{	printf("mfc open fail\n");		exit(1);		}
		
	CLEAR(encoding_set); //�ʱ�ȭ
	init_encoding_set(&encoding_set); //enc_config�� �����ٵ�. �̻��ϱ�. 

	if(SsbSipMfcEncInit(encoding, &encoding_set) != 1)
	{	printf("mfc init failed\n");		exit(1);	}

	if(SsbSipMfcEncGetInBuf(encoding, &input_info) != 1)
	{	printf("mfc get inbuf failed\n"); 	exit(1);	}

	if(SsbSipMfcEncSetInBuf(encoding, &input_info) != 1)
	{	printf("mfc set inbuf failed\n");	exit(1);	}

	init_server(port);
	printf("waiting client");
	
	connect_client();
	printf("connect_client");

	int count = 0 ; 
	SsbSipMfcEncGetOutBuf(encoding, &output_info);
	printf("output_info.headerSize : %d\n",output_info.headerSize);
	/*
	int i;
	unsigned char *offset =(unsigned char *)output_info.StrmVirAddr;
	for( i = 0 ; i < 21; i++)
	{
		printf("%d \n", *offset++);
	}
	*/
	
	send_to_client(output_info.StrmVirAddr,output_info.headerSize);
	printf(" frameType : %d \n", output_info.frameType);

	while(1)
	{	
		v4l2_data =(char *) v4l2_read_frame(); //get yuv420 data
		
		yuy2tonv12(v4l2_data, nv12l, y_size);	//yuy2 -> nv12l
		nv12ltonv12t(nv12l, nv12t, y_size, aligned_y_size);	//nv12l -> nv12t

		memcpy(input_info.YVirAddr, nv12t, aligned_y_size);
		memcpy(input_info.CVirAddr, nv12t + aligned_y_size, aligend_c_size);

		mfc_ret = SsbSipMfcEncExe(encoding);
		printf("========================SSBSIPMFCENCEXE====================\n");
		if(mfc_ret != MFC_RET_OK)
		{
			printf("SsbSipMfcEncExe failed\n");
			break;
			printf("count : %d \n",count++);
		}
		printf("========================SSBSIPMFCENCGETOUTBUF====================\n");
		//SsbSipMfcEncGetOutBuf���� ���� size�� ���ڵ�� �޸𸮸� ���� ������ Ȯ�� �� ��
		SsbSipMfcEncGetOutBuf(encoding, &output_info);
		
		if(output_info.StrmVirAddr == NULL)
		{	printf("StrmPhyAddr is NULL\n");		break;		}
		printf("========================CIENT====================\n");
		int size = output_info.dataSize;
		printf("size: %d \n", size);

		send_to_client(&output_info.dataSize, sizeof(unsigned int));
		send_to_client(output_info.StrmVirAddr, output_info.dataSize);
		printf(" frameType : %d \n", output_info.frameType);
			
		count++;
	}
	
	//���� �� �ʱ�ȭ
	SsbSipMfcEncClose(encoding);
	free(nv12l);
	free(nv12t);
	stop_capturing ();
	uninit_device ();
	close_device ();
	exit (EXIT_SUCCESS);

	return 0;
}
