This is a 3ds arm9-only bootloader, for loading a payload from the raw sectors of multiple devices. This bootloader is intended to be as small as possible, hence why only raw sectors are read here.  

# Building
"make" can be used with the following options:
* "ENABLE_RETURNFROMCRT0=1" When no payload was successfully booted, or when the payload returned, return from the crt0 to the LR from the time of loader entry, instead executing an infinite loop.
* "UNPROTBOOT9_LIBPATH={path}" This should be used to specify the path for the unprotboot9_sdmmc library, aka the path for that repo.

# Boot procedure
Once main_() is reached, after doing sdmmc initialization, it attempts to boot from each of the following devices, in the same order listed below:
* SD
* NAND

For each device it will scan for a valid payload in the first 4MiB of the device image. Whenever *any* errors occur, it will continue scanning with the next sector. The entire payload must be within the already mentioned sector range.

# Payload format
The format of the payload is based on offical FIRM(all 4 sections are supported). The payloadbuilder tool in this repo can be used to build payloads in this format.
* The u32 at offset 0x4 in the header must match 0x742b4187(in official FIRM this is normally 0x0).
* The ARM11 entrypoint must be 0x0(since this bootloader is arm9-only), and the ARM9 entrypoint must be 0x0.
* The sections' offset and size must be 0x200-byte aligned. Address must be 4-byte aligned.
* Like offical FIRM loading code, the sections' hashes are all validated.
* Instead of a RSA signature, a raw sha256 hash from the "signature" over the first 0x100-bytes of the header is validated.
* MPU is disabled and the sections are loaded with CPU-memcopy internally, hence the only memory that a section can't load to is blacklisted memory ranges. These are: all memory used by this loader, areas of ITCM which are used by the system, and the entire DTCM.

