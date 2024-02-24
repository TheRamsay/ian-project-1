/*
 *  ian-proj1.c
 *  Author: Dominik Huml <xhuld00@vutbr.cz>
 *  Date: 22.02.2024
 *  Description: A simple ELF file parser that prints the names and values of global data objects defined by the user.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    Elf *e;

    // Get file descriptor for the ELF file
    if ((fd = open(filename, O_RDONLY, 0)) < 0)
    {
        fprintf(stderr, "Error: unable to open ELF file %s\n", filename);
        return 1;
    }

    // Initialize ELF library
    elf_version(EV_NONE);
    if (elf_version(EV_CURRENT) == EV_NONE)
    {
        fprintf(stderr, "Error: ELF library initialization failed: %s\n", elf_errmsg(-1));
        return 1;
    }

    // Open the ELF file
    if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
    {
        fprintf(stderr, "Error: unable to read ELF file %s\n", filename);
        errx(1, "elf_begin() failed: %s.", elf_errmsg(-1));
        return 1;
    }

    Elf_Scn *scn = NULL;
    GElf_Shdr shdr;
    size_t shstrndx;

    // .symtab section
    Elf_Data *symtab_data = NULL;
    GElf_Shdr symtab_shdr;

    // .data section
    Elf_Data *data_data = NULL;
    GElf_Shdr data_shdr;
    size_t data_shdr_index;
    size_t bss_shdr_index;

    // Get the section header string table index
    if (elf_getshdrstrndx(e, &shstrndx) != 0)
    {
        fprintf(stderr, "Error: unable to get section header string table index\n");
        return 1;
    }

    // Start from the second section because the first section is always NULL
    size_t section_index = 1;
    while ((scn = elf_nextscn(e, scn)) != NULL)
    {
        gelf_getshdr(scn, &shdr);

        if (shdr.sh_type == SHT_SYMTAB)
        {
            symtab_data = elf_getdata(scn, NULL);
            symtab_shdr = shdr;
        }
        else if (strcmp(elf_strptr(e, shstrndx, shdr.sh_name), DATA_SECTION_NAME) == 0)
        {
            data_data = elf_getdata(scn, NULL);
            data_shdr = shdr;
            data_shdr_index = section_index;
        }
        else if (strcmp(elf_strptr(e, shstrndx, shdr.sh_name), BSS_SECTION_NAME) == 0)
        {
            bss_shdr_index = section_index;
        }

        section_index++;
    }

    if (data_shdr_index == 0)
    {
        fprintf(stderr, "Error: unable to find .data section\n");
        return 1;
    }

    if (bss_shdr_index == 0)
    {
        fprintf(stderr, "Error: unable to find .bss section\n");
        return 1;
    }

    if (symtab_data == NULL)
    {
        fprintf(stderr, "Error: unable to find symbol table\n");
        return 1;
    }

    if (data_data == NULL)
    {
        fprintf(stderr, "Error: unable to find .data section\n");
        return 1;
    }

    if (symtab_shdr.sh_entsize == 0)
    {
        fprintf(stderr, "Error: symbol table entry size is 0\n");
        return 1;
    }

    size_t symtab_entries = symtab_shdr.sh_size / symtab_shdr.sh_entsize;

    // Iterate through the symbol table
    for (size_t i = 0; i < symtab_entries; i++)
    {
        GElf_Sym sym;
        gelf_getsym(symtab_data, i, &sym);

        // Filter out only global data objects defiend by the user
        if (
            ELF64_ST_TYPE(sym.st_info) == STT_OBJECT &&                            // Symbol is a data object (variable)
            ELF64_ST_BIND(sym.st_info) == STB_GLOBAL &&                            // Symbol is globally visible
            (sym.st_shndx == bss_shdr_index || sym.st_shndx == data_shdr_index) && // Symbol is in .bss or .data
            sym.st_size != 0                                                       // Symbol has a size, i.e. is allocated
        )
        {
            unsigned long value_address = (unsigned long)sym.st_value;
            unsigned long start_address = (unsigned long)data_shdr.sh_addr;
            unsigned long offset = value_address - start_address;

            int value = 0;

            // If the symbol is in .data section, get the value from the .data section
            // If the symbol is in .bss section, the value is 0, so no need to get it
            if (sym.st_shndx == data_shdr_index)
            {
                value = ((int *)data_data->d_buf)[offset / 4];
            }

            printf("Variable name: %s \t Value: %d\n", elf_strptr(e, symtab_shdr.sh_link, sym.st_name), value);
        }
    }

    // Clean up
    elf_end(e);
    close(fd);

    return 0;
}