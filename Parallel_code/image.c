#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "image.h"
#include "stdio-wrapper.h"

char* strrev(char* str)
{
	char *p1, *p2;
	if (!str || !*str)
		return str;
	for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
	{
		*p1 ^= *p2;
		*p2 ^= *p1;
		*p1 ^= *p2;
	}
	return str;
}

int myatoi (char* string)
{
	int sign = 1;
	// how many characters in the string
	int length = strlen(string);
	int i = 0;
	int number = 0;

	// handle sign
	if (string[0] == '-')
	{
		sign = -1;
		i++;
	}

//	for (i; i < length; i++)
	while(i < length)
	{
		// handle the decimal place if there is one
		if (string[i] == '.')
			break;
		number = number * 10 + (string[i]- 48);
		i++;
	}
	number *= sign;
	return number;
}

void itochar(int x, char* szBuffer, int radix)
{
	int i = 0, n, xx;
	n = x;
	while (n > 0)
	{
		xx = n%radix;
		n = n/radix;
		szBuffer[i++] = '0' + xx;
	}
	szBuffer[i] = '\0';
	strrev(szBuffer);
}

int cpyPgm(MyImage* src, MyImage* dst)
{
	int i = 0;
	if (src->flag == 0)
	{
		printf("No data available in the specified source image\n");
		return -1;
	}
	dst->width = src->width;
	dst->height = src->height;
	dst->maxgrey = src->maxgrey;
	dst->data = (unsigned char*)malloc(sizeof(unsigned char)*(dst->height*dst->width));
	dst->flag = 1;
	for (i = 0; i < (dst->width * dst->height); i++)
	{
		dst->data[i] = src->data[i];
	}
}


void createImage(int width, int height, MyImage *image)
{
	image->width = width;
	image->height = height;
	image->flag = 1;
	image->data = (unsigned char *)malloc(sizeof(unsigned char)*(height*width));
}

void createSumImage(int width, int height, MyIntImage *image)
{
	image->width = width;
	image->height = height;
	image->flag = 1;
	image->data = (int *)malloc(sizeof(int)*(height*width));
}

int freeImage(MyImage* image)
{
	if (image->flag == 0)
	{
		printf("no image to delete\n");
		return -1;
	}
	else
	{
//		printf("image deleted\n");
		free(image->data); 
		return 0;
	}
}

int freeSumImage(MyIntImage* image)
{
	if (image->flag == 0)
	{
		printf("no image to delete\n");
		return -1;
	}
	else
	{
		free(image->data); 
		return 0;
	}
}

void setImage(int width, int height, MyImage *image)
{
	image->width = width;
	image->height = height;
}

void setSumImage(int width, int height, MyIntImage *image)
{
	image->width = width;
	image->height = height;
}
