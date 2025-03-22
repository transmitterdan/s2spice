#include <stdio.h>
#include <math.h>
#define MAXLINE 128
#define MAXFREQS 200
#define MAXPORTS 8
#define MAXNAME 58

/* help; prints help */
void help() {
  printf("Usage: s2spice Touchstone_file [ Z1 Z2 [ Zi... ] ]\n");
  printf(
      "This program obtains the Spice equivalent circuit from the "
      "S-parameters\n");
  printf("The number of ports must be between 1 and 8\n");
}

/* copy: copies 'num' characters from vector to output file */
/* if num==0 copies until \n is found */
void copy(char s[], FILE *ofp, int num, int add) {
  int i, c;
  for (i = 0; s[i] != '\n' && ((i < num) || (num == 0)); i++) putc(s[i], ofp);
  if (add) putc('\n', ofp);
}

void error(int c) {
  switch (c) {
    case 1:
      printf("Number of ports must be between 1 and 8\n");
    case 2:
      printf("# not found in input file\n");
  }
}

double absol(double c) {
  if (c < 0) {
    c = -c;
    return c;
  } else {
    return c;
  }
}

int sgn(double c) {
  if (c < 0) {
    return -1;
  } else {
    return 1;
  }
}

void main(int argc, char *argv[]) {
  int c, i, j, f;
  int ports;
  int funits;
  int cont;
  int numf;
  int fscanf_result;
  char line[MAXLINE] = ".SUBCKT ";
  char line2[MAXLINE] = " 1 2 3 4 5 6 7 8 9 \n";
  char lib[MAXNAME] = "lib\0            ";
  FILE *in, *out;
  double z[MAXPORTS];
  double ph, prevph, offset, mag;
  double s[MAXFREQS][MAXPORTS][MAXPORTS][2];
  double freqs[MAXFREQS];
  char name[MAXNAME];

  /* invoke help */
  /* the program must be called with 2,4...10 arguments */

  if ((argc < 2) || (argc > 10) || (argc == 3)) {
    help();
    goto end;
  }

  /* obtain number of ports */

  if (argc == 2) { /* the Z's are not specified in the command line */
    printf("Number of ports of the structure\n");
    printf("Return for help\n");
    if ((ports = getchar()) == '\n') {
      help();
      goto end;
    }
    /* decode number of ports */
    ports = ports - '0';
    if ((ports == 0) || (ports > MAXPORTS)) {
      error(1);
      goto end;
    }
  } else { /* the Z's are given in the command line */
    ports = argc - 2;
  }
  printf("The circuit has %d ports\n", ports);

  /* open input file */

  i = 0;
  while ((name[i] = *argv[1]) != '\0') {
    i++;
    argv[1]++;
  }

  if ((in = fopen(&name, "r")) == NULL) {
    printf("Can't open file\n");
    goto end;
  }

  /* open output file. Same name as input but with .lib extension */

  while (name[i] != '.') i--;
  j = 0;
  i++;
  while (MAXNAME >= (i + j)) {
    name[i + j] = lib[j];
    j++;
  }

  if ((out = fopen(&name, "w")) == NULL) {
    printf("Can't open file\n");
    goto end;
  }

  /* build first line of output file */

  copy(line, out, 8, 0);
  copy(name, out, i - 1, 0);
  copy(line2, out, 3 + ports * 2, 1);

  /* copy comment lines from input to output files */

  while ((c = getc(in)) == '!') {
    line[0] = '*';
    for (i = 1; ((line[i] = getc(in)) != '\n') && (i < MAXLINE); i++)
      ;
    copy(line, out, MAXLINE, 1);
  }

  /* read and decode format line */

  while ((c != '#') && (c != EOF)) c = getc(in);
  if (c == EOF) {
    error(2);
    goto end;
  } else {
    line[0] = '*';
    for (i = 1; ((line[i] = getc(in)) != '\n') && (i < 11); i++)
      ;
    copy(line, out, 12, 1);
  }
  if (line[2] == 'M') { /* Frequencies in MHz  */
    funits = 1000;
  } else {
    funits = 1;
  }

  /* input impedances */

  fscanf_result = fscanf(in, "%lf", &z[1]);
  for (i = 1; i <= ports; i++) {
    if (argc == 2) { /* takes the Z value from the input file */
      z[i] = z[1];
    } else { /* takes the Z value from the command line */
      fscanf_result = sscanf(argv[1 + i], "%lf", &z[i]);
    }
  }

  /* copies Z's to output file as comments */

  fprintf(out, "*");
  for (i = 1; i <= ports; i++) fprintf(out, " Z%d=%7.6f", i, z[i]);
  fprintf(out, "\n\n");

  /* define resistances for Spice model */

  for (i = 1; i <= ports; i++) {
    fprintf(out, "R%dN %d %d %e\n", i, i, 10 * i, -z[i]);
    fprintf(out, "R%dP %d %d %e\n", i, 10 * i, 10 * i + 1, 2 * z[i]);
  }
  fprintf(out, "\n");

  /* read S parameters into matrix */

  cont = 1;
  for (f = 1; cont; f++) {
    fscanf_result = fscanf(in, "%lf", &freqs[f]);
    if (ports != 2) {
      for (i = 1; i <= ports; i++)
        for (j = 1; j <= ports; j++) {
          fscanf_result = fscanf(in, "%lf", &s[f][i][j][1]);
          fscanf_result = fscanf(in, "%lf", &s[f][i][j][2]);
        }
    } else { /* the input format is different for two ports */
      fscanf_result = fscanf(in, "%lf", &s[f][1][1][1]);
      fscanf_result = fscanf(in, "%lf", &s[f][1][1][2]);
      fscanf_result = fscanf(in, "%lf", &s[f][2][1][1]);
      fscanf_result = fscanf(in, "%lf", &s[f][2][1][2]);
      fscanf_result = fscanf(in, "%lf", &s[f][1][2][1]);
      fscanf_result = fscanf(in, "%lf", &s[f][1][2][2]);
      fscanf_result = fscanf(in, "%lf", &s[f][2][2][1]);
      fscanf_result = fscanf(in, "%lf", &s[f][2][2][2]);
    }

    /* discard commented lines */

    while (((c = getc(in)) != '\n') && (c != EOF))
      ;
    while ((c = getc(in)) == '!')
      while ((c = getc(in)) != '\n')
        ;
    while ((c == ' ') || (c == '\n'))
      c = getc(in); /* discard blank lines at the end */
    if (c == EOF) {
      cont = 0;
      numf = f;
    } else {
      c = ungetc(c, in);
    }
  }

  printf("%d frequency values read\n", numf);

  /* write values to output file*/

  for (i = 1; i <= ports; i++) {
    for (j = 1; j <= ports; j++) {
      fprintf(out, "*S%d%d FREQ DB PHASE\n", i, j);
      if (j == ports) {
        fprintf(out, "E%d%d %d%d %d FREQ {V(%d,%d)}=\n", i, j, i, j, ports + 1,
                10 * j, ports + 1);
      } else {
        fprintf(out, "E%d%d %d%d %d%d FREQ {V(%d,%d)}=\n", i, j, i, j, i, j + 1,
                10 * j, ports + 1);
      }
      offset = 0;
      prevph = 0;
      for (f = 1; f <= numf; f++) {
        mag = 20 * log10(s[f][i][j][1]);
        ph = s[f][i][j][2];
        if ((absol(ph - prevph)) > 180) {
          offset = offset - 360 * sgn(ph - prevph);
        }
        prevph = ph;
        fprintf(out, "+(%14egHz,%14e,%14e)\n", freqs[f] * funits, mag,
                ph + offset);
      }
      fprintf(out, "\n");
    }
  }

  fprintf(out, ".ENDS\n");

  c = fclose(in);
  c = fclose(out);

end:
  printf("Processing Completed\n");
}
