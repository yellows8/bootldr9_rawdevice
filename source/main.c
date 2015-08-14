#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <3ds.h>

#include <unprotboot9_sdmmc.h>

typedef int (*read_funcptr)(u32, u32, u32*);//params: u32 sector, u32 numsectors, u32 *out

typedef struct {
	u32 offset;
	u32 address;
	u32 size;
	u32 type;
	u32 hash[0x20>>2];
} firm_sectionhdr;

int boot_device(u32 device, read_funcptr read_data, u32 basesector, u32 maxsectors)
{
	int ret = 0, imagefound = 0;
	u32 pos, cursector, firmsector, firmindex, curaddr;
	u32 sector0, sector1;

	void (*arm9_entrypoint)();

	//u32 firmhdr[0x200>>2];
	u32 *firmhdr = (u32*)(0x08003120 + device*0x200);

	firm_sectionhdr *section_headers = (firm_sectionhdr*)&firmhdr[0x40>>2];

	//maxsectors = 1;

	for(cursector=0; cursector<maxsectors; cursector++)//Find+load the FIRM image.
	{
		firmsector = basesector+cursector;

		//for(pos=0; pos<(0x200>>2); pos++)firmhdr[pos] = 0;

		ret = unprotboot9_sdmmc_readrawsectors(firmsector, 1, firmhdr);//Only read 1 sector at a time since this code should use as little stack space as possible.
		*((u32*)0x0800311c) = ret;
		if(ret!=0)continue;

		if(firmhdr[0] != 0x4d524946)continue;//Verify the FIRM magicnum.
		if(firmhdr[1] != 0x742b4187)continue;//Verify that firmhdr[1] matches this custom magicnum.
		if(firmhdr[2]==0 || firmhdr[3]!=0)continue;//Since this is only an arm9 bootloader, verify that the FIRM arm11 entrypoint is zero and that the arm9 entrypoint is non-zero.

		arm9_entrypoint = (void*)firmhdr[2];

		ret = 0;

		for(firmindex=0; firmindex<4; firmindex++)
		{
			if(section_headers[firmindex].size==0)continue;

			curaddr = section_headers[firmindex].address;

			if((section_headers[firmindex].offset & 0x1ff) || (section_headers[firmindex].size & 0x1ff) || (curaddr & 0x3))//Check for alignment.
			{
				ret = 4;
				break;
			}

			sector0 = firmsector + (section_headers[firmindex].offset>>9);
			sector1 = sector0 + (section_headers[firmindex].size>>9);

			if((sector0 < firmsector) || (sector1 < firmsector))//Check for integer overflow when the sector values are added together.
			{
				ret = 2;
				break;
			}

			if((sector0 >= basesector+maxsectors) || (sector1 >= basesector+maxsectors))//The section sector values must not go out of bounds with the input params sectors range.
			{
				ret = 3;
				break;
			}

			ret = read_data(sector0, (section_headers[firmindex].size>>9), (u32*)curaddr);

			if(ret!=0)break;
		}

		if(ret!=0)continue;

		imagefound = 1;
		break;
	}

	if(imagefound==0)return 1;

	arm9_entrypoint();

	return 0;
}

int boot_sd()
{
	int ret = 0;
	u32 pos;
	u32 *ptr;

	ret = unprotboot9_sdmmc_initdevice(unprotboot9_sdmmc_deviceid_sd);
	if(ret==0)boot_device(0, (read_funcptr)unprotboot9_sdmmc_readrawsectors, 0x0, 0x400000>>9);

	return ret;
}

int boot_nand()
{
	int ret = 0;

	ret = unprotboot9_sdmmc_initdevice(unprotboot9_sdmmc_deviceid_nand);
	if(ret==0)boot_device(1, (read_funcptr)unprotboot9_sdmmc_readrawsectors, 0x0, 0x400000>>9);

	return ret;
}

void main_()
{
	s32 ret;
	u32 pos;

	ret = unprotboot9_sdmmc_initialize();
	*((u32*)0x08003110) = (u32)ret;
	if(ret)return;

	*((u32*)0x080030fc) = 0x33333333;

	ret = boot_sd();
	*((u32*)0x08003114) = ret;
	//if(ret==0)return;

	//*((u32*)0x08003100) = unprotboot9_sdmmc_readrawsectors(0, 1, (u32*)0x08003120);

	ret = boot_nand();
	*((u32*)0x08003118) = ret;
	//if(ret==0)return;

	//*((u32*)0x08003100) = unprotboot9_sdmmc_readrawsectors(0, 1, (u32*)(0x08003120+0x200));

	*((u32*)0x080030fc) = 0x55555555;
}

