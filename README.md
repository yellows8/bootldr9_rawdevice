This is a 3ds arm9-only bootloader, for loading a payload from the raw sectors of multiple devices. This bootloader is intended to be as small as possible, hence why only raw sectors are read here.  

# Building
"make" can be used with the following options:
* "ENABLE_RETURNFROMCRT0=1" When no payload was successfully booted, or when the payload returned, return from the crt0 to the LR from the time of loader entry, instead executing an infinite loop.
* "UNPROTBOOT9_LIBPATH={path}" This should be used to specify the path for the unprotboot9_sdmmc library, aka the path for that repo.

* "ENABLE_CONTINUEWHEN_PAYLOADCALLFAILS=1" When used, set the retval returned by boot_device() to the value returned from calling arm9_entrypoint(), if that ever returns. This will result in continuing to attempt to boot from the rest of the devices, if that retval from the payload is non-zero.

* "DEVICEDISABLE_SD=1" Completely disables using SD.
* "DEVICEDISABLE_NAND=1" Same as above except for NAND.
* "DEVICEDISABLE_SPIFLASH=1" Same as above except for spiflash.

* "ENABLE_PADCHECK=1" Enables using the below PADCHECK options. Val in the below options is a bitmask which is checked with the PAD register, the PAD register is only read once. PADCHECK_DISABLEDEVICE_{DEVICE} and PADCHECK_ENABLEDEVICE_{DEVICE} for each device, are all the same besides the affected device. PADCHECK_DISABLEDEVICE_{DEVICE} and PADCHECK_ENABLEDEVICE_{DEVICE} can't be used at the same time for the same device.
* "PADCHECK_DISABLEDEVICE_SD={val}" Disable using the device when any of the bits in val are set in the PAD register.
* "PADCHECK_ENABLEDEVICE_SD={val}" Only enable using the device when any of the bits in val are set in the PAD register.
* "PADCHECK_DISABLEDEVICE_NAND={val}" See above.
* "PADCHECK_ENABLEDEVICE_NAND={val}" See above.
* "PADCHECK_DISABLEDEVICE_SPIFLASH={val}" See above.
* "PADCHECK_ENABLEDEVICE_SPIFLASH={val}" See above.

# Boot procedure
Once main_() is reached, after doing sdmmc initialization, it attempts to boot from each of the following devices(when the device is actually enabled), in the same order listed below:
* SD, tested. Sector range: first 4MiB of the device image.
* NAND, tested. Sector range: byte-offset 0x0, end byte-offset 0x12C00.
* Spiflash, tested. Sector range: byte-offset 0x1f400, end byte-offset 0x20000. This is the only region in spiflash which isn't write-protected - the writable region actually begins at 0x1f300 but due to "sector" alignment start-offset 0x1f400 has to be used here.

For each device it will scan for a valid payload in a certain range of sectors, see above. Whenever *any* errors occur, it will continue scanning with the next sector. The entire payload must be within the above sector range.

# Payload format
The format of the payload is based on offical FIRM(all 4 sections are supported). The payloadbuilder tool in this repo can be used to build payloads in this format.
* The u32 at offset 0x4 in the header must match 0x742b4187(in official FIRM this is normally 0x0). The u32 at offset 0x3c in the header must match 0x1c083e7f(signature type).
* The ARM11 entrypoint must be 0x0(since this bootloader is arm9-only), and the ARM9 entrypoint must be 0x0.
* The sections' offset and size must be 0x200-byte aligned. Address must be 4-byte aligned.
* Like offical FIRM loading code, the sections' hashes are all validated.
* Instead of a RSA signature, a raw sha256 hash from the "signature" over the first 0x100-bytes of the header is validated.
* MPU is disabled and the sections are loaded with CPU-memcopy internally, hence the only memory that a section can't load to is blacklisted memory ranges. These are: all memory used by this loader, areas of ITCM which are used by the system, and the entire DTCM.

