#pragma once

/*
  input:  data origin preloader data,
          psz: in/output  data size, in origin preloader size, out: new preloader size.
  return: -1: not a valid preloader. 0 OK.
*/
int process_preloader (char* data, /*IN OUT*/unsigned int* psz);

#define HEADER_BLOCK_SIZE                 (2048)    //2K 0x800
