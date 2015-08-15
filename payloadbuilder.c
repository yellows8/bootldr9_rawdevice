#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

#include <openssl/sha.h>

//Build with: gcc -o payloadbuilder payloadbuilder.c -lcrypto

int main(int argc, char **argv)
{
	FILE *f;
	struct stat filestat;
	unsigned int binaddr = 0;
	size_t arm9bin_size = 0, sectionsize_aligned = 0, tmpsize = 0;
	unsigned char *arm9bin_buf = NULL;
	uint32_t header[0x200>>2];

	if(argc<4)
	{
		printf("payloadbuilder by yellows8\n");
		printf("This builds payloads for use with bootldr9_rawdevice. This is based on the offical 3ds FIRM format. The loader supports all 4 sections, however this tool is hard-coded to only handle the first section.\n");
		printf("Usage:\n");
		printf("payloadbuilder <output payload path> <arm9bin path> <arm9bin load/entrypoint address>\n");
		return 0;
	}

	memset(header, 0, sizeof(header));

	sscanf(argv[3], "0x%x", &binaddr);
	if(binaddr==0)
	{
		printf("Invalid arm9bin address.\n");
		return 1;
	}

	if(stat(argv[2], &filestat)==-1)
	{
		printf("Failed to stat() the arm9bin.\n");
		return 2;
	}

	arm9bin_size = filestat.st_size;
	if(arm9bin_size==0)
	{
		printf("The arm9bin file is empty.\n");
		return 3;
	}

	sectionsize_aligned = (arm9bin_size + 0x1ff) & ~0x1ff;
	arm9bin_buf = malloc(sectionsize_aligned);
	if(arm9bin_buf==NULL)
	{
		printf("Failed to alloc memory for the arm9bin section buffer.\n");
		return 4;
	}
	memset(arm9bin_buf, 0, sectionsize_aligned);

	f = fopen(argv[2], "rb");
	if(f)
	{
		if(fread(arm9bin_buf, 1, arm9bin_size, f) != arm9bin_size)
		{
			printf("Failed to read the arm9bin.\n");
			free(arm9bin_buf);
			fclose(f);
			return 6;
		}
		fclose(f);
	}
	else
	{
		printf("Failed to open the arm9bin.\n");
		free(arm9bin_buf);
		return 5;
	}

	printf("Successfully loaded arm9bin, generating + writing output payload...\n");

	header[0] = 0x4d524946;//Init magic-nums + arm9 entrypoint.
	header[1] = 0x742b4187;
	header[3] = binaddr;

	header[0x40>>2] = 0x200;//Init section0.
	header[0x44>>2] = binaddr;
	header[0x48>>2] = sectionsize_aligned;
	//Leave the "section type" at value 0.
	SHA256(arm9bin_buf, sectionsize_aligned, (unsigned char*)&header[0x50>>2]);

	SHA256((unsigned char*)header, 0x100, (unsigned char*)&header[0x100>>2]);//Init the "signature".

	f = fopen(argv[1], "wb");
	if(f)
	{
		if(fwrite(header, 1, sizeof(header), f) != sizeof(header))
		{
			printf("Failed to write the header to the output payload.\n");
			free(arm9bin_buf);
			fclose(f);
			return 7;
		}

		tmpsize = fwrite(arm9bin_buf, 1, sectionsize_aligned, f);
		free(arm9bin_buf);
		fclose(f);

		if(tmpsize != sectionsize_aligned)
		{
			printf("Failed to write the arm9 section to the output payload.\n");
			return 7;
		}
	}
	else
	{
		printf("Failed to open the output payload.\n");
		free(arm9bin_buf);
		return 7;
	}

	printf("Successfully wrote the output payload.\n");

	return 0;
}

