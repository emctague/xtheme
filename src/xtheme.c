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

typedef enum {
	PropType_Boolean, // A single boolean value
	PropType_String,  // A single string value
	PropType_List     // An array of property (string) values
} PropType;

typedef struct {
	PropType type;            // The type of this property
	const char* name;		  // The property name
	const char* output;       // Output when set to true, or for list items (%2$d = the current number) (%1$s = the value given)
	const char* output_false; // Output for booleans that have been set to false.
} Prop;

// Check if the given `string` starts with `thing`
int startsWith(const char* thing, const char* string) {
  return !strncmp(thing, string, strlen(thing));
}

Prop properties[] = {
	{ PropType_Boolean, "cursor blink", "*cursorBlink: true\n", "*cursorBlink: false\n" },
	{ PropType_Boolean, "use clipboard", "*VT100*Translations: #override \\\n"
      "    Ctrl Shift <Key>V: insert-selection(CLIPBOARD) \\n\\\n"
      "    Ctrl Shift <Key>C: copy-selection(CLIPBOARD)\n", "" },
    { PropType_Boolean, "use scrollbar", "*scrollBar: true\n", "*scrollBar: false\n" },
    { PropType_String, "padding", "*internalBorder: %1$s\n", "" },
    { PropType_String, "color fg", "*foreground: %1$s\n", "" },
    { PropType_String, "color bg", "*background: %1$s\n", "" },
    { PropType_String, "color cursor", "*cursorColor: %1$s\n", "" },
    { PropType_String, "color bold", "*colorBFMode: true\n*colorBD: %1$s\n", "" },
    { PropType_String, "color italic", "*colorITMode: true\n*colorIT: %1$s\n", "" },
    { PropType_String, "color underline", "*colorULMode: true\n*colorUL: %1$s\n", "" },
    { PropType_String, "font face", "*faceName: %s\n", "" },
    { PropType_String, "font size", "*faceSize: %s\n", "" },
    { PropType_List, "colors", "*color%2$d: %1$s\n", "" }
};

int propSize = sizeof(properties) / sizeof(Prop);

int main (int argc, char* argv[]) {
  // Ensure file is supplied
  if (argc != 2) {
    fprintf(stderr, "Usage: %s [file]\n", argv[0]);
    return 1;
  }

  // Build the path to the .Xresources file.
  struct passwd *pw = getpwuid(getuid());
  const char* home = pw->pw_dir;

  const char* file = "/.Xresources";

  char* outpath = malloc(strlen(home) + strlen(file) + 1);
  strcpy(outpath, home);
  strcat(outpath, file);

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

  // Read through each config line.
  while ((read = getline(&line, &length, config)) != -1) {
    line[strlen(line) - 1] = 0;

    for (int i = 0; i < propSize; i++) {
    	int j = 0;

    	if (startsWith(properties[i].name, line)) {
    		switch (properties[i].type) {
			case PropType_String:
				fprintf (out, properties[i].output, &line[strlen(properties[i].name) + 1]);
				goto ctnu;
				break;
			case PropType_Boolean:
				if (!startsWith("false", &line[strlen(properties[i].name) + 1]))
					fprintf (out, "%s", properties[i].output);
				else
					fprintf (out, "%s", properties[i].output_false);
				goto ctnu;
				break;
			case PropType_List:
				while ((read  = getline(&line, &length, config)) != -1) {
					line[strlen(line) - 1] = 0;
					if (!strcmp(line, "")) break;
					fprintf(out, properties[i].output, line, j++);
				}
				goto ctnu;
				break;
    		}
    	}
    }    
    ctnu:
    continue;
  }

  fclose(config);
  fclose(out);
  free(line);

  // Merge.
  system("xrdb -merge ~/.Xresources");
  puts("Updated, restart XTerm to see changes.");

  return 0;
}
