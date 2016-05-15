#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

//mfc
#include "SsbSipMfcApi.h"
#include "mfc_interface.h"
#include "mfc_errorno.h"
#include "video.h"
#define BUFSIZE 100
#define NAMESIZE 20
#define BUFFER_SIZE 1024*1024

void error_handling(char *message);
int buf[BUFSIZE];

//decoding...
void *virbuf=NULL;
void *phybuf=NULL;
unsigned char tilebuf[BUFFER_SIZE];
unsigned short rgb565[BUFFER_SIZE];

int height = 480;
int width = 800;

int main(int argc, char** argv)
{
	///////////////////////////////////////////////////////

	int sock;
	struct sockaddr_in serv_addr;

	if (argc != 3)
	{
		printf("Usage : %s <IP> <PORT> \n", argv[0]);
		exit(1);
	}
	v4l2_init();
	int errCode;
	_MFCLIB *decoding = NULL;
	int mfc_ret;
	unsigned char *decodig_outbuf; //디코딩 출력버프
	decodig_outbuf = (unsigned char *)malloc((height * width * 3) >>1 );

	decoding = SsbSipMfcDecOpen(); //디코딩 디바이스 오픈;
	if(decoding == NULL){
		printf("mfc openfail\n");
		exit(1);
	}
	virbuf = SsbSipMfcDecGetInBuf(decoding , &phybuf, BUFFER_SIZE);

	if(virbuf == NULL){
		printf("decgetinbuf failed\n");
		exit(1);
	}
	
	int set_conf_val = 5 ; 
	SsbSipMfcDecSetConfig(decoding, MFC_DEC_SETCONF_EXTRA_BUFFER_NUM, &set_conf_val);
	
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");
	
	//////////////////DECODING/////////////////////////
	
	unsigned char header_size[21];
	read(sock,header_size, 21);
	/*int n = 0;	
	while(n < 21)
		printf("%d ",header_size[n++]);*/
	
	memcpy(virbuf, header_size, 21); // 해더를 받아서 넣어야함. 
	
	if((errCode = SsbSipMfcDecInit(decoding, H264_DEC, BUFFER_SIZE)) != MFC_RET_OK){
		printf("MFC DEC INIT FAILED\n");
		exit(1);
	}
//H.265fream info recv
//memcopy
	unsigned int encoding_sz=0;
	unsigned char encoding_data[BUFFER_SIZE];
	int count = 0;
	int recv_byte=0;
	memset(tilebuf, 0x00, sizeof(unsigned char) * BUFFER_SIZE); //디코딩 버퍼 
	
	while(1)
	{
		recv_byte=0;
		//printf("ready! data recv!\n");
		read(sock,&encoding_sz,sizeof(unsigned int));
		printf("%d\n",encoding_sz);
		while(recv_byte < encoding_sz)
			recv_byte += read(sock, encoding_data+recv_byte,encoding_sz-recv_byte);
		
		memcpy(virbuf,encoding_data,recv_byte);
		printf("data recv!\n");
		if(count > 10)
		{
			if((errCode = SsbSipMfcDecExe(decoding, encoding_sz)) == MFC_RET_OK)
			{

					SSBSIP_MFC_DEC_OUTBUF_STATUS decoding_status;
					SSBSIP_MFC_DEC_OUTPUT_INFO decoding_output_info;
					int status;
					int imgwidth, imgheight;
					int bufwidth, bufheight;
					unsigned char *p_nv12, *p_cb, *p_cr;
					int i;
					memset(decodig_outbuf, 0x80, (imgwidth * imgheight * 3) >> 1);

					status = SsbSipMfcDecGetOutBuf(decoding, &decoding_output_info);
					
					imgwidth = decoding_output_info.img_width;
					imgheight = decoding_output_info.img_height;
					bufwidth = decoding_output_info.buf_width;
					bufheight = decoding_output_info.buf_height;
					
					printf("imgwidth : %d \n", imgwidth);
					printf("imgheight : %d\n", imgheight);
					printf("bufwidth : %d\n", imgwidth);
					printf("bufheight: %d\n", bufheight);

					memcpy(tilebuf, decoding_output_info.YVirAddr, bufwidth * bufheight);
					tile_to_linear_4x2(decodig_outbuf, tilebuf, imgwidth, imgheight);

					memcpy(tilebuf + bufwidth * bufheight, decoding_output_info.CVirAddr, bufwidth * bufheight);
					tile_to_linear_4x2(decodig_outbuf + imgwidth * imgheight, tilebuf + bufwidth * bufheight, imgwidth, imgheight >> 1);
					
					memcpy(tilebuf, decodig_outbuf + imgwidth * imgheight, (imgwidth * imgheight) >> 1);
					
					p_nv12 = tilebuf;
					p_cb = decodig_outbuf + imgwidth * imgheight;
					p_cr = p_cb;
	 				p_cr += ((imgwidth * imgheight) >> 2);
					
					
					for(i = 0; i < ((imgwidth * imgheight) >> 2); ++i)
					{
						*p_cb = *p_nv12;
						++p_nv12;
						*p_cr = *p_nv12;
						++p_nv12;
						++p_cb;
						++p_cr;
					}
				
					cvt_420p_to_rgb565(800,480,decodig_outbuf,rgb565);
					Print_image(rgb565);
					
					/*
					sprintf(filename,"/mnt/usb0/%d.yuv",count);
					int filedes = open(filename, O_WRONLY | O_CREAT, 0644);
					write(filedes, decodig_outbuf, (imgwidth * imgheight)*1.5);
					*/
	
			}
		}
		else errCode = SsbSipMfcDecExe(decoding, encoding_sz);
		count++;
		}
		
	//////////////////////////////////////////////////////
	
	close(sock);
	//printf("program exit()");

	return 0;
}



void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

void cvt_420p_to_rgb565(int width, int height, const unsigned char *src, unsigned short *dst)
{
  int line, col, linewidth;
  int y, u, v, yy, vr, ug, vg, ub;
  int r, g, b;
  const unsigned char *py, *pu, *pv;

  linewidth = width >> 1;
  py = src;
  pu = py + (width * height);
  pv = pu + (width * height) / 4;

  y = *py++;
  yy = y << 8;
  u = *pu - 128;
  ug = 88 * u;
  ub = 454 * u;
  v = *pv - 128;
  vg = 183 * v;
  vr = 359 * v;

  for (line = 0; line < height; line++) {
    for (col = 0; col < width; col++) {
      r = (yy + vr) >> 8;
      g = (yy - ug - vg) >> 8;
      b = (yy + ub ) >> 8;

      if (r < 0) r = 0;
      if (r > 255) r = 255;
      if (g < 0) g = 0;
      if (g > 255) g = 255;
      if (b < 0) b = 0;
      if (b > 255) b = 255;
      *dst++ = (((unsigned short)r>>3)<<11) | (((unsigned short)g>>2)<<5) | (((unsigned short)b>>3)<<0);
  
      y = *py++;
      yy = y << 8;
      if (col & 1) {
    pu++;
    pv++;

    u = *pu - 128;
    ug = 88 * u;
    ub = 454 * u;
    v = *pv - 128;
    vg = 183 * v;
    vr = 359 * v;
      }
    }
    if ((line & 1) == 0) {
      pu -= linewidth;
      pv -= linewidth;
    }
  }
}
