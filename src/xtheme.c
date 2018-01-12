/* XTheme project - main source file.
 * Copyright (C) 2018 Ethan McTague
 * Licensed under the BSD 2-clause license. See COPYING for license text, and
 * README for more licensing and software information, including documentation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>


// Reading modes.
#define MODE_READ 0  // Currently reading basic config properties.
#define MODE_LIST 1  // Reading list items.

// Types of properties.
#define TYPE_ENABLE 0  // Turns something on, without giving a value.
#define TYPE_SET    1  // Single argument on the same line.
#define TYPE_LIST   2  // Multiple arguments on lines following it.

int startsWith(const char* thing, const char* string) {
  return !strncmp(thing, string, strlen(thing));
}

typedef void (*cbptr)(int, const char*);

// Each property.
typedef struct {
  int type;            // Type int (see "Types of properties" section, above.)
  const char* name;    // Name of this property.
  const char* format;  // Output format string for enable and set types.
  cbptr func; // Callback for TYPE_LIST type.
  int listLength;        // Number of list elements needed.
} property;

#define P(AA, AB, AC, AD, AE) &(property){ AA, AB, AC, AD, AE }

#define BUF_SIZE 128
char buf[BUF_SIZE];

void take_colors (int i, const char* c) {
  snprintf(buf, BUF_SIZE, "*color%d: %s\n", i, c);
}

void take_colors_multi (int i, const char* c) {
  snprintf(buf, BUF_SIZE, "*color%d: %s\n*color%d: %s\n", i, c, i + 8, c);
}

property* properties[] = {
  P(TYPE_ENABLE, "cursor blink", "*cursorBlink: true\n", 0, 0),
  P(TYPE_ENABLE, "use clipboard",
      "*VT100*Translations: #override \\\n"
      "    Ctrl Shift <Key>V: insert-selection(CLIPBOARD) \\n\\\n"
      "    Ctrl Shift <Key>C: copy-selection(CLIPBOARD)\n", 0, 0),
  P(TYPE_SET, "use scrollbar ", "*scrollBar: %s\n", 0, 0),
  P(TYPE_SET, "padding ", "*internalBorder: %s\n", 0, 0),
  P(TYPE_SET, "color fg ", "*foreground: %s\n", 0, 0),
  P(TYPE_SET, "color bg ", "*background: %s\n", 0, 0),
  P(TYPE_SET, "color cursor ", "*cursorColor: %s\n", 0, 0),
  P(TYPE_SET, "color bold ", "*colorBFMode: true\n*colorBD: %s\n", 0, 0),
  P(TYPE_SET, "color italic ", "*colorITMode: true\n*colorIT: %s\n", 0, 0),
  P(TYPE_SET, "color underline ", "*colorULMode: true\n*colorUL: %s\n", 0, 0),
  P(TYPE_SET, "font face ", "*faceName: %s\n", 0, 0),
  P(TYPE_SET, "font size ", "*faceSize: %s\n", 0, 0),
  P(TYPE_LIST, "colors multi", 0, &take_colors_multi, 8),
  P(TYPE_LIST, "colors", 0, &take_colors, 16)
};

int propSize = sizeof(properties) / sizeof(properties[0]);

int main (int argc, char* argv[]) {
  
  // Ensure proper arguments.
  if (argc < 2) {
    fputs("Supply a single argument, the path to the config file.\n", stderr);
    return 1;
  }

  // Build the path to the .Xresources file.
  struct passwd *pw = getpwuid(getuid());
  const char* home = pw->pw_dir;

  const char* file = "/.Xresources";
  const char* file2 = "/.Xdefaults";

  char* outpath = malloc(strlen(home) + strlen(file) + 1);
  strcpy(outpath, home);
  strcat(outpath, file);

  char* outpath2 = malloc(strlen(home) + strlen(file2) + 1);
  strcpy(outpath2, home);
  strcat(outpath2, file2);

  // Open the given config file.
  FILE* config = fopen(argv[1], "r");
  FILE* out = fopen(outpath, "w");

  // Ensure properly opened.
  if (config == NULL) {
    fputs("Could not open configuration file.\n", stderr);
    return 2;
  }

  char* line = NULL;
  size_t length = 0;
  size_t read;

  int mode = MODE_READ;
  int listIndex;
  int listLength;
  cbptr listAction;

  // Read through each config line.
  while ((read = getline(&line, &length, config)) != -1) {

    line[strlen(line) - 1] = 0;

    switch (mode) {

    // Identify options.
    case MODE_READ:
      for (int i = 0; i < propSize; i++) {
        property* p = properties[i];

        switch (p->type) {
        case TYPE_ENABLE:
          if (!strcmp(p->name, line)) {
            fputs(p->format, out);
            goto nextline;
          }
          break;
        case TYPE_SET:
          if (startsWith(p->name, line)) {
            char* target = &line[strlen(p->name)];
            fprintf(out, p->format, target);
            goto nextline;
          }
          break;
        case TYPE_LIST:
          if (!strcmp(p->name, line)) {
            mode = MODE_LIST;
            listAction = p->func;
            listLength = p->listLength;
            listIndex = 0;
            goto nextline;
          }
          break;
        }
      }
      break;


    case MODE_LIST:

      listAction(listIndex, line);
      fputs(buf, out);
      listIndex++;

      if (listIndex >= listLength)
        mode = MODE_READ;

      break;
    }

nextline:
    continue;
    
  }

  fclose(config);
  fclose(out);
  if (line) free(line);

  // Link ~/.Xdefaults to ~/.Xresources
  symlink(outpath, outpath2);

  // Merge.
  system("xrdb -merge ~/.Xresources");

  puts("Updated, restart XTerm to see changes.");

  return 0;
}
