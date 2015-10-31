/*
  Flashtool for AVRUSBBoot, an USB bootloader for Atmel AVR controllers

  Thomas Fischl <tfischl@gmx.de>

  Creation Date..: 2006-03-18
  Last change....: 2006-06-25
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "cflashmem.h"
#include "cbootloader.h"

int main(int argc, char **argv) {

  if (argc < 2) {
    fprintf(stderr, "usage: avrusbboot filename.hex\n");
    exit(1);
  }

  printf("initializing bootloader...\n");
  CBootloader *bootloader = new CBootloader();
  fprintf(stderr, "bootloader initialized\n");

  unsigned int pagesize = bootloader->getPagesize();
  
  printf("Pagesize: %d\n", pagesize);

  CFlashmem * flashmem = new CFlashmem(pagesize);

  flashmem->readFromIHEX(argv[1]);

  CPage* pPage = flashmem->getFirstpage();
  while (pPage != NULL) {
    printf("Write page at adresse: %d\n", pPage->getPageaddress());
    bootloader->writePage(pPage);
    pPage = pPage->getNext();
  } 

  return 0;
}
