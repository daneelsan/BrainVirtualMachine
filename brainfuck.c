#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define  u8  uint8_t
#define u16 uint16_t

//#define MAX_CODE_SIZE 65536;
#define MEMORY_SIZE   30000
#define BUFFER_SIZE    1024

/*
struct BrainProgram {
  size_t length;
  char   *code;
};
*/

struct BrainVM {
  u8   memory[MEMORY_SIZE];
  u16  data_pointer;
  char *code;
  u16  code_length;
  u16  instruction_pointer;
  char input;
  char output;
  
};

void
initialize(struct BrainVM *vm, const char *code)
{
  u16 code_length = (u16)strlen(code);
  vm->code = (char *)malloc(sizeof(char) * code_length + 1);
  for (int i = 0; i < code_length; ++i) {
    vm->code[i] = code[i];
  }
  vm->code[code_length] = '\0';
  vm->code_length = code_length;
}

struct BrainVM
create(const char *code)
{
  struct BrainVM vm;
  
  for (int i = 0; i < MEMORY_SIZE; ++i) {
    vm.memory[i] = 0;
  }
  vm.data_pointer = 0;
  
  initialize(&vm, code);
  vm.instruction_pointer = 0;
  
  vm.input  = '\0';
  vm.output = '\0';
  
  return vm;
}

void
destroy(struct BrainVM *vm)
{
  free(vm->code);
}

int
step(struct BrainVM *vm)
{
  // fetch
  char instruction = vm->code[vm->instruction_pointer];
  // decode & execute
  switch (instruction) {
    case '>': {
      vm->data_pointer++;
    } break;
    case '<': {
      vm->data_pointer--;
    } break;
    case '+': {
      vm->memory[vm->data_pointer]++;
    } break;
    case '-': {
      vm->memory[vm->data_pointer]--;
    } break;
    case '[': {
      // if value of current cell is zero, find matching square bracket
      if (vm->memory[vm->data_pointer] == 0) {
        int depth = 1;
        while (depth != 0) {
          vm->instruction_pointer++;
          switch (vm->code[vm->instruction_pointer]) {
            case  '[':  depth++; break;
            case  ']':  depth--; break;
            case '\0': return 1; break;
          }
        }
      }
    } break;
    case ']': {
      // if value of current cell is not zero, loop back to matching square bracket
      if (vm->memory[vm->data_pointer] != 0) {
        int depth = 1;
        while (depth != 0) {
          vm->instruction_pointer--;
          switch (vm->code[vm->instruction_pointer]) {
            case  '[':  depth--; break;
            case  ']':  depth++; break;
            case '\0': return 1; break;
          }
        }
      }
    } break;
    case ',': {
      char ch = fgetc(stdin);
      vm->input = ch;
      vm->memory[vm->data_pointer] = (u8)ch;
    } break;
    case '.': {
      char ch = (u8)vm->memory[vm->data_pointer];
      vm->output = ch;
      fputc(ch, stdout);
    } break;
  }
  vm->instruction_pointer++;
  
  return 1;
}

void
print_state(struct BrainVM *vm)
{
  fprintf(stderr, "\n[\n");
  fprintf(stderr, "  code:                ");
  u16 ip = vm->instruction_pointer;
  for (int i = ip - 16; i < ip + 16; ++i) {
    if (i < 0) {
      i = -1;
    } else if (i > vm->code_length) {
      //fprintf(stderr, " ");
    } else if (i == ip) {
      fprintf(stderr, "{%c}", vm->code[i]);
    } else {
      fprintf(stderr, "%c", vm->code[i]);
    }
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "  instruction pointer: %d\n", ip);
  
  fprintf(stderr, "  memory:              ");
  u16 dp = vm->data_pointer;
  for (int i = dp - 16; i < dp + 16; ++i) {
    if (i < 0) {
      i = -1;
    } else if (i > MEMORY_SIZE) {
      fprintf(stderr, " ");
    } else if (i == ip) {
      fprintf(stderr, "{%d}", vm->memory[i]);
    } else {
      fprintf(stderr, "[%d]", vm->memory[i]);
    }
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "  data pointer:        %d\n", dp);
  
  fprintf(stderr, "  input:               '%c'\n", vm->input);
  fprintf(stderr, "  output:              '%c'\n", vm->output);
  fprintf(stderr, "]");
  fprintf(stderr, "\n");
}

char *
read_code_from_file(const char *file_name)
{
  FILE *fp;
  fp = fopen(file_name, "r");
  if (fp == NULL) {
    perror("Error in opening file");
    exit(1);
  }
  
  int ch;
  size_t code_length = 0, buffer_size = BUFFER_SIZE;
  char *code = malloc(sizeof(char) * buffer_size);
  
  while ((ch = fgetc(fp)) != EOF) {
    switch (ch) {
      case '#': {
        do { // ignore anything after '#'
          ch = fgetc(fp);
        } while (ch != '\n' && ch != EOF);
      } break;
      case '\n':
      case ' ': { // ignore white spaces to avoid large code size
      } break;
      default: { // an alternative is to only accept the 8 instructions
        code[code_length++] = ch;
      } break;
    }
    // https://stackoverflow.com/questions/38621938/using-fgetc-for-reading-into-an-array-in-c
    if (code_length == BUFFER_SIZE) { // need more space
      void *tmp = realloc(code, code_length + BUFFER_SIZE);
      if (!tmp) { // realloc might fail
        fprintf(stderr, "realloc() error: not enough memory.\n");
        break;
      }
      code = tmp;
      buffer_size += BUFFER_SIZE;
    }
  }
  code[code_length] = '\0';
  
  fclose(fp);
  return code;
}

int
main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;
  
  int c;
  opterr = 0;
  
  int debug = 0;
  
  while ((c = getopt(argc, argv, "d:")) != -1) {
    switch (c) {
      case 'd': {
        debug = 1;
      } break;
    }
  }
  
  const char *code = read_code_from_file("program/hello_world.brain");
  
  struct BrainVM vm = create(code);
  
  int run = 1;
  while (run) {
    if (vm.instruction_pointer >= vm.code_length) break;
    if (debug) { print_state(&vm); }
    run = step(&vm);
  }
  destroy(&vm);
  
  return 0;
}
