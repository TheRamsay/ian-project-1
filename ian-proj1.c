/*
 *  ian-proj1.c
 *  Author: Dominik Huml <xhumld00@vutbr.cz>
 *  Date: 22.02.2024
 *  Description: A simple ELF file parser that prints the names and values of global data objects defined by the user.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// File handling
#include <fcntl.h>
#include <unistd.h>

// ELF library
#include <libelf.h>
#include <gelf.h>

// Error handling
#include <sysexits.h>
#include <err.h>

#define DATA_SECTION_NAME ".data"
#define BSS_SECTION_NAME ".bss"

int main(int argc, char *argv[])
{
    // Check for correct number of arguments (program_name <elf-file>)
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <elf-file>\n", argv[0]);
        return 1;
    }

    // Get the ELF file name
    char *filename = argv[1];
    int fd;
    Elf *elf_file;

    // Get file descriptor for the ELF file
    if ((fd = open(filename, O_RDONLY, 0)) < 0)
    {
        fprintf(stderr, "Error: unable to open ELF file %s\n", filename);
        return EXIT_FAILURE;
    }

    // Initialize ELF library
    elf_version(EV_NONE);
    if (elf_version(EV_CURRENT) == EV_NONE)
    {
        close(fd);
        fprintf(stderr, "Error: ELF library initialization failed: %s\n", elf_errmsg(-1));
        return EXIT_FAILURE;
    }

    // Open the ELF file
    if ((elf_file = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
    {
        fprintf(stderr, "Error: unable to read ELF file %s\n", filename);
        errx(1, "elf_begin() failed: %s.", elf_errmsg(-1));
        close(fd);
        return EXIT_FAILURE;
    }

    // Structures for elf sections
    Elf_Scn *scn = NULL;
    GElf_Shdr shdr;
    size_t shstrndx;

    // .symtab section
    Elf_Data *symtab_data = NULL;
    GElf_Shdr symtab_shdr;

    // .data section
    Elf_Data *data_section_data = NULL;
    GElf_Shdr data_section_shdr;
    size_t data_section_shdr_index = 0;

    // .bss section
    size_t bss_section_shdr_index = 0;

    // Get the section header string table index
    if (elf_getshdrstrndx(elf_file, &shstrndx) != 0)
    {
        fprintf(stderr, "Error: unable to get section header string table index\n");
        goto cleanup;
    }

    // Start from the second section because the first section is always NULL
    size_t section_index = 1;
    while ((scn = elf_nextscn(elf_file, scn)) != NULL)
    {
        // Check for uninitialized variable that gcc warns about
        // This ensures that `shdr` is filled with the section header information, if returned value is NULL => error
        if (gelf_getshdr(scn, &shdr) == NULL)
        {
            fprintf(stderr, "Error: unable to get section header\n");
            goto cleanup;
        }

        // We are interested only in .symtab, .data and .bss sections
        if (shdr.sh_type == SHT_SYMTAB) // Symbol table
        {
            symtab_data = elf_getdata(scn, NULL);
            symtab_shdr = shdr;
        }
        else if (strcmp(elf_strptr(elf_file, shstrndx, shdr.sh_name), DATA_SECTION_NAME) == 0) // .data section
        {
            data_section_data = elf_getdata(scn, NULL);
            data_section_shdr = shdr;
            data_section_shdr_index = section_index;
        }
        else if (strcmp(elf_strptr(elf_file, shstrndx, shdr.sh_name), BSS_SECTION_NAME) == 0) // .bss section
        {
            bss_section_shdr_index = section_index;
        }

        // If we have found all the required sections, we can break the loop
        if (symtab_data != NULL && data_section_data != NULL && data_section_shdr_index != 0 && bss_section_shdr_index != 0)
        {
            break;
        }

        section_index++;

    }

    // .data section is required
    if (data_section_shdr_index == 0 || data_section_data == NULL)
    {
        fprintf(stderr, "Error: unable to find .data section\n");
        goto cleanup;
    }

    // .symtab section is required
    if (symtab_data == NULL)
    {
        fprintf(stderr, "Error: unable to find symbol table\n");
        goto cleanup;
    }

    // Check if the symbol table entry size is not 0
    if (symtab_shdr.sh_entsize == 0 || symtab_shdr.sh_size == 0)
    {
        fprintf(stderr, "Error: symbol table entry size is 0\n");
        goto cleanup;
    }

    size_t symtab_entries = symtab_shdr.sh_size / symtab_shdr.sh_entsize;

    // Iterate through the symbol table
    for (size_t i = 0; i < symtab_entries; i++)
    {
        GElf_Sym sym;
        gelf_getsym(symtab_data, i, &sym);

        // Filter out only global data objects defiend by the user
        if (
            ELF64_ST_TYPE(sym.st_info) == STT_OBJECT &&                                            // Symbol is a data object (variable)
            ELF64_ST_BIND(sym.st_info) == STB_GLOBAL &&                                            // Symbol is globally visible
            (
                (bss_section_shdr_index != 0 ? sym.st_shndx == bss_section_shdr_index : false) ||  // Symbol is in .bss, if .bss is not present, ignore this condition
                (sym.st_shndx == data_section_shdr_index)                                          // Symbol is in .data
            ) &&
            sym.st_size != 0                                                                       // Symbol has a size, i.e. is allocated
        )
        {
            // Calculate the offset of the symbol in the .data section
            unsigned long value_address = (unsigned long)sym.st_value;
            unsigned long start_address = (unsigned long)data_section_shdr.sh_addr;
            unsigned long offset = value_address - start_address;

            // Default value is 0, in case the symbol is uninitialized
            int value = 0;

            // If the symbol is in .data section, get the value from the .data section
            // If the symbol is in .bss section, the value is 0, so no need to get it
            if (sym.st_shndx == data_section_shdr_index)
            {
                if (sym.st_size == sizeof(char))
                {
                    value = ((char*)data_section_data->d_buf)[offset];
                }
                else if (sym.st_size == sizeof(short))
                {
                    value = ((short*)data_section_data->d_buf)[offset / sizeof(short)];
                }
                else if (sym.st_size == sizeof(int))
                {
                    value = ((int*)data_section_data->d_buf)[offset / sizeof(int)];
                }
            }
        
            if (sym.st_size == 1)
            {
                printf("Variable: %s\t\tValue: %c\tSize: %ld\n", elf_strptr(elf_file, symtab_shdr.sh_link, sym.st_name), value, sym.st_size);
            }
            else if (sym.st_size == 2)
            {
                printf("Variable: %s\t\tValue: %hu\tSize: %ld\n", elf_strptr(elf_file, symtab_shdr.sh_link, sym.st_name), value, sym.st_size);
            }
            else if (sym.st_size == 4)
            {
                printf("Variable: %s\t\tValue: %d\tSize: %ld\n", elf_strptr(elf_file, symtab_shdr.sh_link, sym.st_name), value, sym.st_size);
            }
        }
    }

    elf_end(elf_file);
    close(fd);
    return EXIT_SUCCESS;

cleanup:
    elf_end(elf_file);
    close(fd);

    return EXIT_FAILURE;
}