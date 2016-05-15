/*
*  V4L2 video capture example
*
*  This program can be used and distributed without restrictions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include <asm/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>

#define CLIP(t) (((t)>255)?255:(((t)<0)?0:(t)))
#define CLEAR(x) memset (&(x), 0, sizeof (x))
#define FBDEV_FILE "/dev/fb0"
	
typedef enum {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
} io_method;

struct buffer {
	void *                  start;
	size_t                  length;
};

unsigned char *ptr_casting = NULL;
static char* dev_name = "/dev/video/preview";
static io_method io = IO_METHOD_MMAP;
static int fd = -1;
struct buffer* buffers = NULL;
static unsigned int n_buffers = 0;
static int fbfd;
static int ret;
static int memsize;
static int i, j;
static unsigned short *pfbdata;
static struct fb_var_screeninfo fbvar;
static struct fb_fix_screeninfo fbfix;


void errno_exit(const char *s)
{
	fprintf (stderr, "%s error %d, %s\n",
		s, errno, strerror (errno));
	exit (EXIT_FAILURE);
}

int xioctl(int fd, int request, void *arg)
{
	int r;
	do r = ioctl (fd, request, arg);
	while (-1 == r && EINTR == errno);
	return r;
}

void stop_capturing(void)
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
		errno_exit ("VIDIOC_STREAMOFF");
}

void uninit_device(void)
{
	unsigned int i;
	for (i = 0; i < n_buffers; ++i)
		if (-1 == munmap (buffers[i].start, buffers[i].length))
			errno_exit ("munmap");
	free (buffers);
}

void close_device(void)
{
	if (-1 == close (fd))
		errno_exit ("close");
	fd = -1;
}

void Print_image(unsigned short* ptr)
{
	unsigned short *rec_ptr;

	for(i=0; i<fbvar.yres; i++)
	{
		rec_ptr = (unsigned short*)pfbdata + (fbvar.xres*i);
		for(j=0; j<fbvar.xres; j++)
			*rec_ptr++ = ptr[j+i*fbvar.xres];
	}

}

void process_image(const void *p)
{
	printf("process_image start\n");
	ptr_casting = (unsigned short*)p;
	Print_image(p); 
	printf("end start\n");
	
}

void* v4l2_read_frame(void)
{
	//printf("read_frame start \n");
	struct v4l2_buffer buf;
	unsigned int i;
	void *yuv420fmt;

	CLEAR (buf);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
		switch (errno) {
		case EAGAIN:
			return 0;
		case EIO:
			/* Could ignore EIO, see spec. */

			/* fall through */
		default:
			errno_exit ("VIDIOC_DQBUF");
		}
	}
	assert (buf.index < n_buffers);
	
	yuv420fmt = buffers[buf.index].start;
	
	//process_image (buffers[buf.index].start);

	if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
		errno_exit ("VIDIOC_QBUF");

	//printf("read_frame end \n");

	return yuv420fmt;
}


void v4l2_start_capturing(void)
{
	unsigned int i;
	enum v4l2_buf_type type;

	//printf("n_buffers : %d\n", n_buffers);
	for (i = 0; i < n_buffers; ++i) {
		struct v4l2_buffer buf;
		CLEAR (buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = i;

		if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
			errno_exit ("VIDIOC_QBUF");
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
			errno_exit ("VIDIOC_STREAMON");
}

void init_mmap(void)
{
	struct v4l2_requestbuffers req;
	CLEAR (req);

	req.count               = 4;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;

	if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support "
				"memory mapping\n", dev_name);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf (stderr, "Insufficient buffer memory on %s\n",
			dev_name);
		exit (EXIT_FAILURE);
	}

	buffers = calloc (req.count, sizeof (*buffers));

	if (!buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR (buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = n_buffers;

		if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
			errno_exit ("VIDIOC_QUERYBUF");

		//printf("buf length : %d\n", buf.length);

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start =
			mmap (NULL /* start anywhere */,
			buf.length,
			PROT_READ | PROT_WRITE /* required */,
			MAP_SHARED /* recommended */,
			fd, buf.m.offset);
		//printf("length : %d\n", buf.length);

		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit ("mmap");
	}
}

void v4l2_init_device(void)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;

	unsigned int min;

	if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s is no V4L2 device\n",
				dev_name);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf (stderr, "%s is no video capture device\n",
			dev_name);
		exit (EXIT_FAILURE);
	}


	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		fprintf (stderr, "%s does not support streaming i/o\n", dev_name);
			exit (EXIT_FAILURE);
		}
	/* Select video input, video standard and tune here. */
	CLEAR (cropcap);
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	} else {        
		/* Errors ignored. */
	}
	CLEAR (fmt);
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = 800; 
	fmt.fmt.pix.height      = 480;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field       = V4L2_FIELD_NONE;
	//printf("kkpidentity %d/%d\n", fmt.fmt.pix.height, fmt.fmt.pix.width);

	if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt))
		errno_exit ("VIDIOC_S_FMT");

	/* Note VIDIOC_S_FMT may change width and height. */

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;	
		min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	init_mmap ();
}

void v4l2_open_device(void)
{
	struct stat st; 
	if (-1 == stat (dev_name, &st)) {
		fprintf (stderr, "Cannot identify '%s': %d, %s\n",
			dev_name, errno, strerror (errno));
		exit (EXIT_FAILURE);
	}
	if (!S_ISCHR (st.st_mode)) {
		fprintf (stderr, "%s is no device\n", dev_name);
		exit (EXIT_FAILURE);
	}
	fd = open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);
	if (-1 == fd) {
		fprintf (stderr, "Cannot open '%s': %d, %s\n",
			dev_name, errno, strerror (errno));
		exit (EXIT_FAILURE);
	}

}

void v4l2_fb_init(void){
	fbfd = open(FBDEV_FILE, O_RDWR); // Dev open
	if(fbfd < 0)
	{
		perror("fbdev open");
		exit(1);
	}

	ret = ioctl(fbfd, FBIOGET_VSCREENINFO, &fbvar);
	fbvar.bits_per_pixel = 16;
	fbvar.yres_virtual = 480;

	if(ioctl(fbfd, FBIOPUT_VSCREENINFO, &fbvar) < 0)
	{
		printf("ppp\n");
		exit(1);	
	}

	if(fbvar.bits_per_pixel != 16)
	{
		fprintf(stderr, "bpp is not 16\n");
		exit(1);
	}

	if(ioctl(fbfd, FBIOGET_FSCREENINFO, &fbfix))
	{
		printf("sss\n");
		exit(1);
	}
	memsize = fbvar.xres*fbvar.yres*2;	
	pfbdata = (unsigned short *)mmap(0, memsize, PROT_READ|PROT_WRITE, MAP_SHARED, fbfd, 0);
	if((unsigned)mmap == (unsigned)-1)
	{
		perror("mmap");
		exit(1);
	}
}

void v4l2_init(void){
	v4l2_fb_init();
	v4l2_open_device();
	v4l2_init_device();
	//printf("init complet\n");
}

