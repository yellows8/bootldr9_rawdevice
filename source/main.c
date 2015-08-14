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

extern u32 _start, __end__;

s32 boot_device(u32 device, read_funcptr read_data, u32 basesector, u32 maxsectors)
{
	s32 ret = 0, imagefound = 0, entrypoint_firmsection_found;
	u32 pos, cursector, firmsector, firmindex, curaddr, checkaddr;
	u32 sector0, sector1;
	u32 *errorptr = (u32*)(0x08003120 + device*0x214);

	void (*arm9_entrypoint)();

	u32 *firmhdr = &errorptr[0x14>>2];

	firm_sectionhdr *section_headers = (firm_sectionhdr*)&firmhdr[0x40>>2];

	u32 firmsections_memrangeblacklist[6][2] = {//Blacklist all memory used by this loader, and also the area used for exception vectors / etc + used areas of ITCM.
	{0x080030fc, 0x080038fc},
	{_start, __end__},
	{0x08100000-0x1000, 0x08100000},//stack
	{0x08000000, 0x08000040},
	{0xfff00000, 0xfff04000},
	{0x3800, 0x7470}//Maked ITCM addrs, resulting in offsets within ITCM.
	};

	for(pos=0; pos<5; pos++)errorptr[pos] = 0x44444444;

	for(cursector=0; cursector<maxsectors; cursector++)//Find+load the FIRM image.
	{
		firmsector = basesector+cursector;

		for(pos=0; pos<(0x200>>2); pos++)firmhdr[pos] = 0;

		ret = read_data(firmsector, 1, firmhdr);
		if(ret!=0)
		{
			errorptr[0] = ret;
			continue;
		}

		if(firmhdr[0] != 0x4d524946)continue;//Verify the FIRM magicnum.

		if(firmhdr[1] != 0x742b4187)
		{
			errorptr[0] = 0x11;
			continue;//Verify that firmhdr[1] matches this custom magicnum.
		}

		if(firmhdr[2]!=0 || firmhdr[3]==0)
		{
			errorptr[0] = 0x12;
			continue;//Since this is only an arm9 bootloader, verify that the FIRM arm11 entrypoint is zero and that the arm9 entrypoint is non-zero.
		}

		arm9_entrypoint = (void*)firmhdr[3];

		errorptr[0] = 0;
		ret = 0;

		entrypoint_firmsection_found = 0;//Verify that the arm9entrypoint is within one of the sections.
		for(firmindex=0; firmindex<4; firmindex++)
		{
			curaddr = section_headers[firmindex].address;
			if(curaddr <= ((u32)arm9_entrypoint) && ((u32)arm9_entrypoint) < curaddr+section_headers[firmindex].size)
			{
				entrypoint_firmsection_found = 1;
				if((curaddr + section_headers[firmindex].size) < curaddr)ret = 0x14;//Check for integer overflow with sectionaddr+sectionsize, for the section where the arm9entrypoint is located.
			}
		}

		if(!entrypoint_firmsection_found || ret!=0)
		{
			if(ret==0)ret = 0x13;
			errorptr[0] = ret;
			continue;
		}

		for(firmindex=0; firmindex<4; firmindex++)
		{
			if(section_headers[firmindex].size==0)continue;

			curaddr = section_headers[firmindex].address;

			if((section_headers[firmindex].offset & 0x1ff) || (section_headers[firmindex].size & 0x1ff) || (curaddr & 0x3))//Check for alignment.
			{
				ret = 0x20;
				break;
			}

			sector0 = firmsector + (section_headers[firmindex].offset>>9);
			sector1 = sector0 + (section_headers[firmindex].size>>9);

			if((sector0 < firmsector) || (sector1 < firmsector))//Check for integer overflow when the sector values are added together.
			{
				ret = 0x21;
				break;
			}

			if((sector0 >= basesector+maxsectors) || (sector1 >= basesector+maxsectors))//The section sector values must not go out of bounds with the input params sectors range.
			{
				ret = 0x22;
				break;
			}

			if((curaddr + section_headers[firmindex].size) < curaddr)//Check for integer overflow with sectionaddr+sectionsize.
			{
				ret = 0x23;
				break;
			}

			ret = 0;
			for(pos=0; pos<6; pos++)
			{
				checkaddr = curaddr;
				if(pos==5)checkaddr &= 0x7fff;

				if(checkaddr >= firmsections_memrangeblacklist[pos][0] && checkaddr < firmsections_memrangeblacklist[pos][1])
				{
					ret = 0x24;
					break;
				}

				if((checkaddr+section_headers[firmindex].size) >= firmsections_memrangeblacklist[pos][0] && (checkaddr+section_headers[firmindex].size) < firmsections_memrangeblacklist[pos][1])
				{
					ret = 0x24;
					break;
				}

				if(checkaddr < firmsections_memrangeblacklist[pos][0] && (checkaddr+section_headers[firmindex].size) > firmsections_memrangeblacklist[pos][0])
				{
					ret = 0x24;
					break;
				}
			}
			if(ret!=0)break;

			ret = read_data(sector0, (section_headers[firmindex].size>>9), (u32*)curaddr);

			errorptr[1+firmindex] = ret;

			if(ret!=0)
			{
				for(pos=0; pos<(section_headers[firmindex].size>>2); pos++)((u32*)curaddr)[pos] = 0;
			}

			if(ret!=0)break;
		}

		if(ret!=0)
		{
			errorptr[1+firmindex] = ret;
			continue;
		}

		imagefound = 1;
		break;
	}

	if(imagefound==0)return 0x10;

	arm9_entrypoint();

	return 0;
}

s32 boot_sd()
{
	s32 ret = 0;

	ret = unprotboot9_sdmmc_initdevice(unprotboot9_sdmmc_deviceid_sd);
	if(ret==0)ret = boot_device(0, (read_funcptr)unprotboot9_sdmmc_readrawsectors, 0x0, 0x400000>>9);

	return ret;
}

s32 boot_nand()
{
	s32 ret = 0;

	ret = unprotboot9_sdmmc_initdevice(unprotboot9_sdmmc_deviceid_nand);
	if(ret==0)ret = boot_device(1, (read_funcptr)unprotboot9_sdmmc_readrawsectors, 0x0, 0x400000>>9);

	return ret;
}

void main_()
{
	s32 ret;

	ret = unprotboot9_sdmmc_initialize();
	*((u32*)0x08003110) = (u32)ret;
	if(ret)return;

	ret = boot_sd();
	*((u32*)0x08003114) = ret;
	if(ret==0)return;

	ret = boot_nand();
	*((u32*)0x08003118) = ret;
	if(ret==0)return;
}

