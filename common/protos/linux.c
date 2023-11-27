#include <stdint.h>
#include <stddef.h>
#include <stdnoreturn.h>
#include <protos/linux.h>
#include <fs/file.h>
#include <lib/libc.h>
#include <lib/misc.h>
#include <lib/term.h>
#include <lib/config.h>
#include <lib/print.h>
#include <lib/uri.h>
#include <mm/pmm.h>
#include <lib/fb.h>
#include <sys/cpu.h>

#if defined (__x86_64__) || defined (__i386__)
#include <lib/acpi.h>
#include <lib/real.h>
#include <drivers/edid.h>
#include <drivers/vga_textmode.h>
#include <drivers/gop.h>
#include <sys/idt.h>

noreturn void linux_spinup(void *entry, void *boot_params);

// The following definitions and struct were copied and adapted from Linux
// kernel headers released under GPL-2.0 WITH Linux-syscall-note
// allowing their inclusion in non GPL compliant code.

#define EDD_MBR_SIG_MAX 16
#define E820_MAX_ENTRIES_ZEROPAGE 128
#define EDDMAXNR 6

struct setup_header {
    uint8_t    setup_sects;
    uint16_t    root_flags;
    uint32_t    syssize;
    uint16_t    ram_size;
    uint16_t    vid_mode;
    uint16_t    root_dev;
    uint16_t    boot_flag;
    uint16_t    jump;
    uint32_t    header;
    uint16_t    version;
    uint32_t    realmode_swtch;
    uint16_t    start_sys_seg;
    uint16_t    kernel_version;
    uint8_t    type_of_loader;
    uint8_t    loadflags;
    uint16_t    setup_move_size;
    uint32_t    code32_start;
    uint32_t    ramdisk_image;
    uint32_t    ramdisk_size;
    uint32_t    bootsect_kludge;
    uint16_t    heap_end_ptr;
    uint8_t    ext_loader_ver;
    uint8_t    ext_loader_type;
    uint32_t    cmd_line_ptr;
    uint32_t    initrd_addr_max;
    uint32_t    kernel_alignment;
    uint8_t    relocatable_kernel;
    uint8_t    min_alignment;
    uint16_t    xloadflags;
    uint32_t    cmdline_size;
    uint32_t    hardware_subarch;
    uint64_t    hardware_subarch_data;
    uint32_t    payload_offset;
    uint32_t    payload_length;
    uint64_t    setup_data;
    uint64_t    pref_address;
    uint32_t    init_size;
    uint32_t    handover_offset;
    uint32_t    kernel_info_offset;
} __attribute__((packed));

struct screen_info {
    uint8_t  orig_x;        /* 0x00 */
    uint8_t  orig_y;        /* 0x01 */
    uint16_t ext_mem_k;    /* 0x02 */
    uint16_t orig_video_page;    /* 0x04 */
    uint8_t  orig_video_mode;    /* 0x06 */
    uint8_t  orig_video_cols;    /* 0x07 */
    uint8_t  flags;        /* 0x08 */
    uint8_t  unused2;        /* 0x09 */
    uint16_t orig_video_ega_bx;/* 0x0a */
    uint16_t unused3;        /* 0x0c */
    uint8_t  orig_video_lines;    /* 0x0e */
    uint8_t  orig_video_isVGA;    /* 0x0f */
    uint16_t orig_video_points;/* 0x10 */

    /* VESA graphic mode -- linear frame buffer */
    uint16_t lfb_width;    /* 0x12 */
    uint16_t lfb_height;    /* 0x14 */
    uint16_t lfb_depth;    /* 0x16 */
    uint32_t lfb_base;        /* 0x18 */
    uint32_t lfb_size;        /* 0x1c */
    uint16_t cl_magic, cl_offset; /* 0x20 */
    uint16_t lfb_linelength;    /* 0x24 */
    uint8_t  red_size;        /* 0x26 */
    uint8_t  red_pos;        /* 0x27 */
    uint8_t  green_size;    /* 0x28 */
    uint8_t  green_pos;    /* 0x29 */
    uint8_t  blue_size;    /* 0x2a */
    uint8_t  blue_pos;        /* 0x2b */
    uint8_t  rsvd_size;    /* 0x2c */
    uint8_t  rsvd_pos;        /* 0x2d */
    uint16_t vesapm_seg;    /* 0x2e */
    uint16_t vesapm_off;    /* 0x30 */
    uint16_t pages;        /* 0x32 */
    uint16_t vesa_attributes;    /* 0x34 */
    uint32_t capabilities;     /* 0x36 */
    uint32_t ext_lfb_base;    /* 0x3a */
    uint8_t  _reserved[2];    /* 0x3e */
} __attribute__((packed));

#define VIDEO_TYPE_MDA        0x10    /* Monochrome Text Display    */
#define VIDEO_TYPE_CGA        0x11    /* CGA Display             */
#define VIDEO_TYPE_EGAM        0x20    /* EGA/VGA in Monochrome Mode    */
#define VIDEO_TYPE_EGAC        0x21    /* EGA in Color Mode        */
#define VIDEO_TYPE_VGAC        0x22    /* VGA+ in Color Mode        */
#define VIDEO_TYPE_VLFB        0x23    /* VESA VGA in graphic mode    */

#define VIDEO_TYPE_PICA_S3    0x30    /* ACER PICA-61 local S3 video    */
#define VIDEO_TYPE_MIPS_G364    0x31    /* MIPS Magnum 4000 G364 video  */
#define VIDEO_TYPE_SGI          0x33    /* Various SGI graphics hardware */

#define VIDEO_TYPE_TGAC        0x40    /* DEC TGA */

#define VIDEO_TYPE_SUN          0x50    /* Sun frame buffer. */
#define VIDEO_TYPE_SUNPCI       0x51    /* Sun PCI based frame buffer. */

#define VIDEO_TYPE_PMAC        0x60    /* PowerMacintosh frame buffer. */

#define VIDEO_TYPE_EFI        0x70    /* EFI graphic mode        */

#define VIDEO_FLAGS_NOCURSOR    (1 << 0) /* The video mode has no cursor set */

#define VIDEO_CAPABILITY_SKIP_QUIRKS    (1 << 0)
#define VIDEO_CAPABILITY_64BIT_BASE    (1 << 1)    /* Frame buffer base is 64-bit */

struct apm_bios_info {
    uint16_t    version;
    uint16_t    cseg;
    uint32_t    offset;
    uint16_t    cseg_16;
    uint16_t    dseg;
    uint16_t    flags;
    uint16_t    cseg_len;
    uint16_t    cseg_16_len;
    uint16_t    dseg_len;
};

struct ist_info {
    uint32_t signature;
    uint32_t command;
    uint32_t event;
    uint32_t perf_level;
};

struct sys_desc_table {
    uint16_t length;
    uint8_t  table[14];
};

struct olpc_ofw_header {
    uint32_t ofw_magic;    /* OFW signature */
    uint32_t ofw_version;
    uint32_t cif_handler;    /* callback into OFW */
    uint32_t irq_desc_table;
} __attribute__((packed));

struct edid_info {
    unsigned char dummy[128];
};

struct efi_info {
    uint32_t efi_loader_signature;
    uint32_t efi_systab;
    uint32_t efi_memdesc_size;
    uint32_t efi_memdesc_version;
    uint32_t efi_memmap;
    uint32_t efi_memmap_size;
    uint32_t efi_systab_hi;
    uint32_t efi_memmap_hi;
};

struct boot_e820_entry {
    uint64_t addr;
    uint64_t size;
    uint32_t type;
} __attribute__((packed));

struct edd_device_params {
    uint16_t length;
    uint16_t info_flags;
    uint32_t num_default_cylinders;
    uint32_t num_default_heads;
    uint32_t sectors_per_track;
    uint64_t number_of_sectors;
    uint16_t bytes_per_sector;
    uint32_t dpte_ptr;        /* 0xFFFFFFFF for our purposes */
    uint16_t key;        /* = 0xBEDD */
    uint8_t device_path_info_length;    /* = 44 */
    uint8_t reserved2;
    uint16_t reserved3;
    uint8_t host_bus_type[4];
    uint8_t interface_type[8];
    union {
        struct {
            uint16_t base_address;
            uint16_t reserved1;
            uint32_t reserved2;
        } __attribute__ ((packed)) isa;
        struct {
            uint8_t bus;
            uint8_t slot;
            uint8_t function;
            uint8_t channel;
            uint32_t reserved;
        } __attribute__ ((packed)) pci;
        /* pcix is same as pci */
        struct {
            uint64_t reserved;
        } __attribute__ ((packed)) ibnd;
        struct {
            uint64_t reserved;
        } __attribute__ ((packed)) xprs;
        struct {
            uint64_t reserved;
        } __attribute__ ((packed)) htpt;
        struct {
            uint64_t reserved;
        } __attribute__ ((packed)) unknown;
    } interface_path;
    union {
        struct {
            uint8_t device;
            uint8_t reserved1;
            uint16_t reserved2;
            uint32_t reserved3;
            uint64_t reserved4;
        } __attribute__ ((packed)) ata;
        struct {
            uint8_t device;
            uint8_t lun;
            uint8_t reserved1;
            uint8_t reserved2;
            uint32_t reserved3;
            uint64_t reserved4;
        } __attribute__ ((packed)) atapi;
        struct {
            uint16_t id;
            uint64_t lun;
            uint16_t reserved1;
            uint32_t reserved2;
        } __attribute__ ((packed)) scsi;
        struct {
            uint64_t serial_number;
            uint64_t reserved;
        } __attribute__ ((packed)) usb;
        struct {
            uint64_t eui;
            uint64_t reserved;
        } __attribute__ ((packed)) i1394;
        struct {
            uint64_t wwid;
            uint64_t lun;
        } __attribute__ ((packed)) fibre;
        struct {
            uint64_t identity_tag;
            uint64_t reserved;
        } __attribute__ ((packed)) i2o;
        struct {
            uint32_t array_number;
            uint32_t reserved1;
            uint64_t reserved2;
        } __attribute__ ((packed)) raid;
        struct {
            uint8_t device;
            uint8_t reserved1;
            uint16_t reserved2;
            uint32_t reserved3;
            uint64_t reserved4;
        } __attribute__ ((packed)) sata;
        struct {
            uint64_t reserved1;
            uint64_t reserved2;
        } __attribute__ ((packed)) unknown;
    } device_path;
    uint8_t reserved4;
    uint8_t checksum;
} __attribute__ ((packed));

struct edd_info {
    uint8_t device;
    uint8_t version;
    uint16_t interface_support;
    uint16_t legacy_max_cylinder;
    uint8_t legacy_max_head;
    uint8_t legacy_sectors_per_track;
    struct edd_device_params params;
} __attribute__ ((packed));

struct boot_params {
    struct screen_info screen_info;            /* 0x000 */
    struct apm_bios_info apm_bios_info;        /* 0x040 */
    uint8_t  _pad2[4];                    /* 0x054 */
    uint64_t  tboot_addr;                /* 0x058 */
    struct ist_info ist_info;            /* 0x060 */
    uint64_t acpi_rsdp_addr;                /* 0x070 */
    uint8_t  _pad3[8];                    /* 0x078 */
    uint8_t  hd0_info[16];    /* obsolete! */        /* 0x080 */
    uint8_t  hd1_info[16];    /* obsolete! */        /* 0x090 */
    struct sys_desc_table sys_desc_table; /* obsolete! */    /* 0x0a0 */
    struct olpc_ofw_header olpc_ofw_header;        /* 0x0b0 */
    uint32_t ext_ramdisk_image;            /* 0x0c0 */
    uint32_t ext_ramdisk_size;                /* 0x0c4 */
    uint32_t ext_cmd_line_ptr;                /* 0x0c8 */
    uint8_t  _pad4[116];                /* 0x0cc */
    struct edid_info edid_info;            /* 0x140 */
    struct efi_info efi_info;            /* 0x1c0 */
    uint32_t alt_mem_k;                /* 0x1e0 */
    uint32_t scratch;        /* Scratch field! */    /* 0x1e4 */
    uint8_t  e820_entries;                /* 0x1e8 */
    uint8_t  eddbuf_entries;                /* 0x1e9 */
    uint8_t  edd_mbr_sig_buf_entries;            /* 0x1ea */
    uint8_t  kbd_status;                /* 0x1eb */
    uint8_t  secure_boot;                /* 0x1ec */
    uint8_t  _pad5[2];                    /* 0x1ed */
    /*
     * The sentinel is set to a nonzero value (0xff) in header.S.
     *
     * A bootloader is supposed to only take setup_header and put
     * it into a clean boot_params buffer. If it turns out that
     * it is clumsy or too generous with the buffer, it most
     * probably will pick up the sentinel variable too. The fact
     * that this variable then is still 0xff will let kernel
     * know that some variables in boot_params are invalid and
     * kernel should zero out certain portions of boot_params.
     */
    uint8_t  sentinel;                    /* 0x1ef */
    uint8_t  _pad6[1];                    /* 0x1f0 */
    struct setup_header hdr;    /* setup header */    /* 0x1f1 */
    uint8_t  _pad7[0x290-0x1f1-sizeof(struct setup_header)];
    uint32_t edd_mbr_sig_buffer[EDD_MBR_SIG_MAX];    /* 0x290 */
    struct boot_e820_entry e820_table[E820_MAX_ENTRIES_ZEROPAGE]; /* 0x2d0 */
    uint8_t  _pad8[48];                /* 0xcd0 */
    struct edd_info eddbuf[EDDMAXNR];        /* 0xd00 */
    uint8_t  _pad9[276];                /* 0xeec */
} __attribute__((packed));

// End of Linux code
#elif defined(__aarch64__)
#include <libfdt/libfdt.h>

// Taken from https://www.kernel.org/doc/Documentation/arm64/booting.txt

struct kernel_header {
    uint32_t code0; /* Executable code */
    uint32_t code1; /* Executable code */
    uint64_t text_offset; /* Image load offset, little endian */
    uint64_t image_size; /* Effective Image size, little endian */
    uint64_t flags; /* kernel flags, little endian */
    uint64_t res2; /* = 0, reserved */
    uint64_t res3; /* = 0, reserved */
    uint64_t res4; /* = 0, reserved */
    uint32_t magic; /* = 0x644d5241, Magic number, little endian, "ARM\x64" */
    uint32_t res5; /* reserved (used for PE COFF offset) */
};

struct linux_efi_initrd {
    unsigned long base;
    unsigned long size;
};

// Linux specifies that the maximum FDT size is 2M.
#define MAX_FDT_SIZE 0x200000
#endif

noreturn void linux_load(char *config, char *cmdline) {
#if defined(__riscv64__)
    (void)cmdline;
#endif

    struct file_handle *kernel_file;

    char *kernel_path = config_get_value(config, 0, "KERNEL_PATH");
    if (kernel_path == NULL)
        panic(true, "linux: KERNEL_PATH not specified");

    print("linux: Loading kernel `%#`...\n", kernel_path);

    if ((kernel_file = uri_open(kernel_path)) == NULL)
        panic(true, "linux: Failed to open kernel with path `%#`. Is the path correct?", kernel_path);

#if defined (__x86_64__) || defined (__i386__)
    uint32_t signature;
    fread(kernel_file, &signature, 0x202, sizeof(uint32_t));

    // validate signature
    if (signature != 0x53726448) {
        panic(true, "linux: Invalid kernel signature");
    }

    size_t setup_code_size = 0;
    fread(kernel_file, &setup_code_size, 0x1f1, 1);

    if (setup_code_size == 0)
        setup_code_size = 4;

    setup_code_size *= 512;

    size_t real_mode_code_size = 512 + setup_code_size;

    struct boot_params *boot_params = ext_mem_alloc(sizeof(struct boot_params));

    struct setup_header *setup_header = &boot_params->hdr;

    size_t setup_header_end = ({
        uint8_t x;
        fread(kernel_file, &x, 0x201, 1);
        0x202 + x;
    });

    fread(kernel_file, setup_header, 0x1f1, setup_header_end - 0x1f1);

    printv("linux: Boot protocol: %u.%u\n",
           setup_header->version >> 8, setup_header->version & 0xff);

    if (setup_header->version < 0x203) {
        panic(true, "linux: Protocols < 2.03 are not supported");
    }

    setup_header->cmd_line_ptr = (uint32_t)(uintptr_t)cmdline;

    // vid_mode. 0xffff means "normal"
    setup_header->vid_mode = 0xffff;

    if (verbose) {
        char *kernel_version = ext_mem_alloc(128);
        if (setup_header->kernel_version != 0) {
            fread(kernel_file, kernel_version, setup_header->kernel_version + 0x200, 128);
            print("linux: Kernel version: %s\n", kernel_version);
        }
        pmm_free(kernel_version, 128);
    }

    setup_header->type_of_loader = 0xff;

    if (!(setup_header->loadflags & (1 << 0))) {
        panic(true, "linux: Kernels that load at 0x10000 are not supported");
    }

    setup_header->loadflags &= ~(1 << 5);     // print early messages

    // load kernel
    uintptr_t kernel_load_addr = 0x100000;
    for (;;) {
        if (memmap_alloc_range(kernel_load_addr,
                ALIGN_UP(kernel_file->size - real_mode_code_size, 4096),
                MEMMAP_BOOTLOADER_RECLAIMABLE, MEMMAP_USABLE, false, false, false))
            break;

        kernel_load_addr += 0x100000;
    }
    fread(kernel_file, (void *)kernel_load_addr, real_mode_code_size, kernel_file->size - real_mode_code_size);
#elif defined(__aarch64__)
    struct kernel_header header;
    fread(kernel_file, &header, 0, sizeof(struct kernel_header));
    if (header.magic != 0x644d5241) {
        panic(true, "linux: Invalid kernel signature");
    }

    // Allocate physical memory for the kernel image
    void *kernel = ext_mem_alloc(header.image_size);
    fread(kernel_file, kernel, 0, header.image_size);
    printv("linux: Kernel allocated at %X\n", (uintptr_t)kernel);

    // Clean and invalidate caches for the memory range the kernel is loaded in
    uintptr_t kernel_addr = (uintptr_t)kernel;
    clean_dcache_poc(kernel_addr, kernel_addr + header.image_size);
    inval_icache_pou(kernel_addr, kernel_addr + header.image_size);

    // Get current exception level
    uint64_t el;
    asm volatile("mrs %0, currentel" : "=r"(el));

    el >>= 2;
    el &= 0x3;

    uint64_t sctlr;
    if (el == 1) {
        printv("linux: Booting from EL1\n");
        asm volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
    } else if (el == 2) {
        printv("linux: Booting from EL2 with virtualization support\n");
        asm volatile("mrs %0, sctlr_el2" : "=r"(sctlr));
    } else {
        panic(true, "linux: Linux cannot be booted from EL3 or EL0");
    }

    // Disable MMU and caches
    sctlr &= ~((1 << 0) | (1 << 2) | (1 << 12));

    if (el == 1) {
        asm volatile("msr sctlr_el1, %0" :: "r"(sctlr));
    } else if (el == 2) {
        asm volatile("msr sctlr_el2, %0" :: "r"(sctlr));
    }

    // Look for the DTB in the configuration tables
    void *fdt_addr = NULL;
    const EFI_GUID dtb_guid = EFI_DTB_TABLE_GUID;

    for (size_t i = 0; i < gST->NumberOfTableEntries; i++) {
        EFI_CONFIGURATION_TABLE *cur_table = &gST->ConfigurationTable[i];
        if (memcmp(&cur_table->VendorGuid, &dtb_guid, sizeof(EFI_GUID)) == 0) {
            fdt_addr = cur_table->VendorTable;
            break;
        }
    }

    // Allocate memory for the FDT and copy it if it was found
    // Otherwise, create an empty tree
    void *fdt = ext_mem_alloc(MAX_FDT_SIZE);
    if (fdt_addr) {
        uint32_t fdt_size = fdt_totalsize(fdt_addr);
        memcpy(fdt, fdt_addr, fdt_size);
        print("linux: Copied %d bytes of FDT\n", fdt_size);
    } else {
        fdt_create_empty_tree(fdt, MAX_FDT_SIZE);
    }

    // Find the /chosen node, or create it if it doesn't exist
    int node = fdt_subnode_offset(fdt, 0, "chosen");
    if (node < 0) {
        node = fdt_add_subnode(fdt, 0, "chosen");
        if (node < 0) {
            panic(true, "linux: Failed to add /chosen node to FDT");
        }
    }

    int status;
    if (!cmdline) {
        goto no_cmdline;
    }

    // Set the bootargs property on the /chosen node
    size_t cmdline_size = strlen(cmdline);
    if (cmdline_size) {
        status = fdt_setprop(fdt, node, "bootargs", cmdline, cmdline_size + 1);
        if (status != 0) {
            goto fdt_error;
        }
    }

no_cmdline:;
    // Setup the /chosen node properties for UEFI
    // Right now they are just placeholder values, but they are updated later

    uint32_t fdt_val32 = UINT32_MAX;
    uint64_t fdt_val64 = UINT64_MAX;

    status = fdt_setprop(fdt, node, "linux,uefi-system-table", &fdt_val64, sizeof(uint64_t));
    if (status != 0) {
        goto fdt_error;
    }

    status = fdt_setprop(fdt, node, "linux,uefi-mmap-start", &fdt_val64, sizeof(uint64_t));
    if (status != 0) {
        goto fdt_error;
    }

    status = fdt_setprop(fdt, node, "linux,uefi-mmap-size", &fdt_val32, sizeof(uint32_t));
    if (status != 0) {
        goto fdt_error;
    }

    status = fdt_setprop(fdt, node, "linux,uefi-mmap-desc-size", &fdt_val32, sizeof(uint32_t));
    if (status != 0) {
        goto fdt_error;
    }

    status = fdt_setprop(fdt, node, "linux,uefi-mmap-desc-ver", &fdt_val32, sizeof(uint32_t));
    if (status != 0) {
        goto fdt_error;
    }
#elif defined(__riscv64)
    panic(true, "linux: RISC-V is not supported yet");
#endif

    fclose(kernel_file);

    ///////////////////////////////////////
    // Modules
    ///////////////////////////////////////

    size_t size_of_all_modules = 0;

    for (size_t i = 0; ; i++) {
        char *module_path = config_get_value(config, i, "MODULE_PATH");
        if (module_path == NULL)
            break;

        struct file_handle *module;
        if ((module = uri_open(module_path)) == NULL)
            panic(true, "linux: Failed to open module with path `%s`. Is the path correct?", module_path);

        size_of_all_modules += module->size;

        fclose(module);
    }

    if (size_of_all_modules == 0) {
        goto no_modules;
    }

#if defined(__x86_64__) || defined(__i386__)
    uint32_t modules_mem_base = setup_header->initrd_addr_max;
    if (modules_mem_base == 0)
        modules_mem_base = 0x38000000;

    modules_mem_base -= size_of_all_modules;
    modules_mem_base = ALIGN_DOWN(modules_mem_base, 4096);

    for (;;) {
        if (memmap_alloc_range(modules_mem_base, ALIGN_UP(size_of_all_modules, 4096),
                               MEMMAP_BOOTLOADER_RECLAIMABLE, MEMMAP_USABLE, false, false, false))
            break;
        modules_mem_base -= 4096;
    }
#else
    uintptr_t modules_mem_base = (uintptr_t)ext_mem_alloc(size_of_all_modules);
    if (modules_mem_base == 0) {
        panic(true, "linux: Failed to allocate memory for modules");
    }
#endif

    uintptr_t _modules_mem_base = modules_mem_base;

    for (size_t i = 0; ; i++) {
        char *module_path = config_get_value(config, i, "MODULE_PATH");
        if (module_path == NULL)
            break;

        print("linux: Loading module `%#`...\n", module_path);

        struct file_handle *module;
        if ((module = uri_open(module_path)) == NULL)
            panic(true, "linux: Could not open `%#`", module_path);

        fread(module, (void *)_modules_mem_base, 0, module->size);

        _modules_mem_base += module->size;
    }

#if defined(__x86_64__) || defined(__i386__)
    setup_header->ramdisk_image = (uint32_t)modules_mem_base;
    setup_header->ramdisk_size  = (uint32_t)size_of_all_modules;
#elif defined(__aarch64__)
    fdt_val64 = cpu_to_fdt64(_modules_mem_base);
    status = fdt_setprop(fdt, node, "linux,initrd-start", &fdt_val64, sizeof(uint64_t));
    if (status != 0) {
        goto fdt_error;
    }

    fdt_val64 = cpu_to_fdt64(ALIGN_UP(_modules_mem_base, 4096));
    status = fdt_setprop(fdt, node, "linux,initrd-end", &fdt_val64, sizeof(uint64_t));
    if (status != 0) {
        goto fdt_error;
    }

    struct linux_efi_initrd *initrd_tbl = ext_mem_alloc(sizeof(struct linux_efi_initrd));
    initrd_tbl->base = _modules_mem_base;
    initrd_tbl->size = size_of_all_modules;

    EFI_GUID linux_initrd_guid = {0x5568e427, 0x68fc, 0x4f3d, {0xac, 0x74, 0xca, 0x55, 0x52, 0x31, 0xcc, 0x68}};
    if (gBS->InstallConfigurationTable(&linux_initrd_guid, initrd_tbl) != EFI_SUCCESS) {
        panic(true, "linux: Failed to install initrd table");
    }
#endif

no_modules:;

    ///////////////////////////////////////
    // Video
    ///////////////////////////////////////

    term_notready();

#if defined(__x86_64__) || defined(__i386__)
    struct screen_info *screen_info = &boot_params->screen_info;
#endif

#if defined (BIOS)
    {
    char *textmode_str = config_get_value(config, 0, "TEXTMODE");
    bool textmode = textmode_str != NULL && strcmp(textmode_str, "yes") == 0;
    if (textmode) {
        goto set_textmode;
    }
    }
#endif

    size_t req_width = 0, req_height = 0, req_bpp = 0;

    char *resolution = config_get_value(config, 0, "RESOLUTION");
    if (resolution != NULL)
        parse_resolution(&req_width, &req_height, &req_bpp, resolution);

    struct fb_info *fbs;
    size_t fbs_count;
#if defined (UEFI) && (defined (__x86_64__) || defined (__i386__))
    gop_force_16 = true;
#endif
    fb_init(&fbs, &fbs_count, req_width, req_height, req_bpp);
    if (fbs_count == 0) {
#if defined (UEFI)
        goto no_fb;
#elif defined (BIOS)
set_textmode:;
        vga_textmode_init(false);

        screen_info->orig_video_mode = 3;
        screen_info->orig_video_ega_bx = 3;
        screen_info->orig_video_lines = 25;
        screen_info->orig_video_cols = 80;
        screen_info->orig_video_points = 16;

        screen_info->orig_video_isVGA = VIDEO_TYPE_VGAC;
#endif
    } else {
#if defined(__x86_64__) || defined(__i386__)
        screen_info->capabilities   = VIDEO_CAPABILITY_64BIT_BASE | VIDEO_CAPABILITY_SKIP_QUIRKS;
        screen_info->flags          = VIDEO_FLAGS_NOCURSOR;
        screen_info->lfb_base       = (uint32_t)fbs[0].framebuffer_addr;
        screen_info->ext_lfb_base   = (uint32_t)(fbs[0].framebuffer_addr >> 32);
        screen_info->lfb_size       = fbs[0].framebuffer_pitch * fbs[0].framebuffer_height;
        screen_info->lfb_width      = fbs[0].framebuffer_width;
        screen_info->lfb_height     = fbs[0].framebuffer_height;
        screen_info->lfb_depth      = fbs[0].framebuffer_bpp;
        screen_info->lfb_linelength = fbs[0].framebuffer_pitch;
        screen_info->red_size       = fbs[0].red_mask_size;
        screen_info->red_pos        = fbs[0].red_mask_shift;
        screen_info->green_size     = fbs[0].green_mask_size;
        screen_info->green_pos      = fbs[0].green_mask_shift;
        screen_info->blue_size      = fbs[0].blue_mask_size;
        screen_info->blue_pos       = fbs[0].blue_mask_shift;

        if (fbs[0].edid != NULL) {
            memcpy(&boot_params->edid_info, fbs[0].edid, sizeof(struct edid_info_struct));
        }

#if defined (BIOS)
        screen_info->orig_video_isVGA = VIDEO_TYPE_VLFB;
#elif defined (UEFI)
        screen_info->orig_video_isVGA = VIDEO_TYPE_EFI;
#endif
#endif
    }

#if defined (UEFI)
no_fb:;
#endif

#if defined(__x86_64__) || defined(__i386__)
    ///////////////////////////////////////
    // RSDP
    ///////////////////////////////////////

    boot_params->acpi_rsdp_addr = (uintptr_t)acpi_get_rsdp();
#endif

    ///////////////////////////////////////
    // UEFI
    ///////////////////////////////////////
#if defined (UEFI)
    efi_exit_boot_services();

#if defined (__x86_64__) || defined (__i386__)
#if defined (__i386__)
    memcpy(&boot_params->efi_info.efi_loader_signature, "EL32", 4);
#else
    memcpy(&boot_params->efi_info.efi_loader_signature, "EL64", 4);
#endif

    boot_params->efi_info.efi_systab    = (uint32_t)(uint64_t)(uintptr_t)gST;
    boot_params->efi_info.efi_systab_hi = (uint32_t)((uint64_t)(uintptr_t)gST >> 32);
    boot_params->efi_info.efi_memmap    = (uint32_t)(uint64_t)(uintptr_t)efi_mmap;
    boot_params->efi_info.efi_memmap_hi = (uint32_t)((uint64_t)(uintptr_t)efi_mmap >> 32);
    boot_params->efi_info.efi_memmap_size     = efi_mmap_size;
    boot_params->efi_info.efi_memdesc_size    = efi_desc_size;
    boot_params->efi_info.efi_memdesc_version = efi_desc_ver;
#elif defined(__aarch64__)
    node = fdt_path_offset(fdt, "/chosen");
    if (node < 0) {
        panic(true, "linux: Failed to find /chosen node in FDT");
    }

    fdt_val64 = cpu_to_fdt64((uintptr_t)gST);
    status = fdt_setprop_inplace(fdt, node, "linux,uefi-system-table", &fdt_val64, sizeof(uint64_t));
    if (status != 0) {
        goto fdt_error;
    }

    fdt_val64 = cpu_to_fdt64((uintptr_t)efi_mmap);
    status = fdt_setprop_inplace(fdt, node, "linux,uefi-mmap-start", &fdt_val64, sizeof(uint64_t));
    if (status != 0) {
        goto fdt_error;
    }

    fdt_val32 = cpu_to_fdt32(efi_mmap_size);
    status = fdt_setprop_inplace(fdt, node, "linux,uefi-mmap-size", &fdt_val32, sizeof(uint32_t));
    if (status != 0) {
        goto fdt_error;
    }

    fdt_val32 = cpu_to_fdt32(efi_desc_size);
    status = fdt_setprop_inplace(fdt, node, "linux,uefi-mmap-desc-size", &fdt_val32, sizeof(uint32_t));
    if (status != 0) {
        goto fdt_error;
    }

    fdt_val32 = cpu_to_fdt32(efi_desc_ver);
    status = fdt_setprop_inplace(fdt, node, "linux,uefi-mmap-desc-ver", &fdt_val32, sizeof(uint32_t));
    if (status != 0) {
        goto fdt_error;
    }
#endif
#endif

#if defined(__x86_64__) || defined(__i386__)
    ///////////////////////////////////////
    // e820
    ///////////////////////////////////////

    struct boot_e820_entry *e820_table = boot_params->e820_table;

    size_t mmap_entries;
    struct memmap_entry *mmap = get_raw_memmap(&mmap_entries);

    for (size_t i = 0, j = 0; i < mmap_entries; i++) {
        if (mmap[i].type >= 0x1000) {
            continue;
        }
        e820_table[j].addr = mmap[i].base;
        e820_table[j].size = mmap[i].length;
        e820_table[j].type = mmap[i].type;
        j++;
        boot_params->e820_entries = j;
    }
#endif

    ///////////////////////////////////////
    // Spin up
    ///////////////////////////////////////

#if defined(__x86_64__) || defined(__i386__)
    irq_flush_type = IRQ_PIC_ONLY_FLUSH;

    common_spinup(linux_spinup, 2, (uint32_t)kernel_load_addr,
                                   (uint32_t)(uintptr_t)boot_params);
#elif defined(__aarch64__)
    fdt_pack(fdt);
    print("linux: FDT allocated at %X\n", (uintptr_t)fdt);

    void(*kernel_entry)(void *, uint64_t, uint64_t, uint64_t) = kernel;
    kernel_entry(fdt, 0, 0, 0);

fdt_error:;
    panic(true, "linux: Failed to update the FDT");
#endif
}
