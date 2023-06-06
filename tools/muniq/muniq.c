#include <alloca.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

//#define __DEBUG__
/*
 * 本程序用于多行去重
 * 如有文件内容为
 * aaa
 * 1111
 * 222
 * 1111
 * 222
 * 3333
 * 去重得到
 * aaa
 * 1111
 * 222
 *
 * 去重是优先从最大行为单位进行，所以一次
 * 运行程序得到的输出文件很可能依旧有重复
 * 行，可以结合 -m -n 和 muniq.sh 控制去重
 * 程度
 */

char *scan_repeat_str(char *p, char *, int, int);

	int
main(int argc, char *argv[])
{
	int max_repeat, opt, min_repeat;
	const char *input_filename;
	const char *output_filename;

	min_repeat = 1;
	max_repeat = 10;
	output_filename = NULL;
	input_filename = NULL;
	while ((opt = getopt(argc, argv, "n:m:o:")) != -1) {
		switch (opt) {
			case 'n':
				min_repeat = atoi(optarg);
				break;
			case 'm':
				max_repeat = atoi(optarg);
				break;
			case 'o':
				output_filename = optarg;
				break;
			default: /* '?' */
				fprintf(stderr, "Usage: %s [-n min-repeat-num] [-m max-repeat-num] <-o output-file> input-name\n",
						argv[0]);
				return -1;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "Expected argument after options\n");
		fprintf(stderr, "Usage: %s [-n min-repeat-num] [-m max-repeat-num] <-o output-file> input-name\n",
				argv[0]);
		return -1;
	}

	input_filename = argv[optind];

	if (output_filename == NULL) {
		fprintf(stderr, "Usage: %s [-n min-repeat-num] [-m max-repeat-num] <-o output-file> input-name\n",
				argv[0]);
		return -1;
	}

#ifdef __DEBUG__
	printf("input_filename : %s, output_filename : %s\n", input_filename, output_filename);
#endif

	int fd;
	if ((fd = open(input_filename, O_RDWR)) < 0) {
		perror("open input_filename : ");
		return -1;
	}

	struct stat st;
	char *data;
	int input_filelen;

	stat(input_filename, &st);
	input_filelen = st.st_size;
	if ((data = mmap(NULL, input_filelen, 
					PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
		perror("mmap input_filename : ");
		return -1;
	}
	close(fd);

#ifdef __DEBUG__
	printf("inpufile len : %d\n", input_filelen);
#endif


	char *pin, *tmp, *pout, *pend;
	int output_filelen;

	pin = data;
	pout = data;
	pend = pin + input_filelen;
	if (output_filename) {
		if ((fd = open(output_filename, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) {
			perror("open output_filename");
			return -1;
		}
		output_filelen = input_filelen;
		ftruncate(fd, output_filelen);
		if ((data = mmap(NULL, output_filelen, 
						PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
			perror("mmap output_filename");
			return -1;
		}
		close(fd);	

		pout = data;
	}

	unsigned long len, total_len;
	int is_repeat;

	total_len = 0;
	while (pin < pend) {
		is_repeat = 1;
		tmp = scan_repeat_str(pin, pend, max_repeat, min_repeat);
		if (tmp == NULL) {
			if ((tmp = strchr(pin, '\n')) == NULL)
				tmp = pend;	
			is_repeat = 0;
		}

		len = tmp - pin + 1;
		total_len += len;
		memcpy(pout, pin, len);
		pout += len;
		pin += len;

		if (is_repeat)
			pin += len;
	}

	if (pin && tmp == NULL && pend > pin) {
		len = pend - pin;
		total_len += len;
		memcpy(pout, pin, len);
#ifdef __DEBUG__
		printf("copy end\n");
#endif
	}

	truncate(output_filename, total_len);

	if (total_len < input_filelen)
		return 1;

	return 0;
}


	char *
strnchr(char *str, char c, unsigned int n)
{
	char *end;

	for (end = str + n; *str != c && str < end; str++)
		NULL;
	if (str == end)
		return NULL;
	return str;
}

	char *
_scan_repeat_str(char *p, char *end, int max_repeat_line, int min_repeat_line, char **lines_mark)
{
	char *p1, *p2, *p3;

	if (max_repeat_line == 0)
		return NULL;

	if (max_repeat_line < min_repeat_line)
		return NULL;

	p1 = p;
	p2 = lines_mark[max_repeat_line - 1];
	p3 = lines_mark[max_repeat_line * 2 - 1];

	if (p2 == NULL)
		goto __next__;

	if (p3 == NULL)
		p3 = end;

	if (p1 == p2)
		return NULL;

	if (p3 - p2 != p2 - p1 + 1)
		goto __next__;

	if (memcmp(p1, p2 + 1, p2 - p1) != 0)
		goto __next__;

	return p2;

__next__:
	return _scan_repeat_str(p, end, max_repeat_line - 1, min_repeat_line, lines_mark);
}

	char *
scan_repeat_str(char *p, char *end, int max_repeat_line, int min_repeat_line)
{
	if (p >= end)
		return NULL;

	char **lines_mark;

	lines_mark = alloca((max_repeat_line*2 + 1) * sizeof(*lines_mark));
	memset(lines_mark, 0x0, sizeof(*lines_mark) * (max_repeat_line*2 + 1));

	int i;
	char *begin, *tmp;
	begin = p;
	for (i = 0; i < max_repeat_line*2 + 1; i++) {
		lines_mark[i] = strnchr(begin, '\n', (unsigned int)(end - begin));
		if (lines_mark[i] == NULL)
			break;
		begin = lines_mark[i] + 1;
	}

	return _scan_repeat_str(p, end, max_repeat_line, min_repeat_line, lines_mark);
}
