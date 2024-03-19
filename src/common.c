//������ ����� ��������� ��������

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "common.h"

 //��������� ����� ��� ����� ��������
int tinybasic_strcmp (char *a, char *b) {
  do {
    if (toupper (*a) != toupper (*b))
      return (toupper (*a) > toupper (*b))
        - (toupper (*a) < toupper (*b));
    else {
      a++;
      b++;
    }
  } while (*a && *b);
  return 0;
}

