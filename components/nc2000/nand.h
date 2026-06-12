#include "comm.h"

extern char nand_magic[11];

uint8_t read_nand();
void nand_write(uint8_t);

void read_nand0_file();
void read_nand_file();

void write_nand0_file(string file="");
void write_nand_file(string file="");

/* Tab5 port: flush dirty NAND cache slots back to the .nand SD file. */
void nand_flush_all();
