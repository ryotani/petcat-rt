#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "petcat.h"
#include "vegcat.h"
#include "neigh.h"
#define MAX_EVT 100

struct evtList {
  char *filename;
  int seg;
  Mario *rawEvts;
  int numEvts;
} runList[2] = {{"../coinc-data/WFOUT-Run0126Segment15.dat", 15, 0, 0},
                {"../coinc-data/WFOUT-Run0126Segment9.dat", 9, 0, 0}};

struct evtList pList[] = {{"none", 15, 0, 1},
                        {"none", 9, 0, 1},
                        {"none", -1, 0, 1}};

float scratch[37][300];


Mario *avg(struct evtList *x, int baselineFlag) {

  struct evtList *y;
  Mario *avgEvt, *evt;
  float avg;
  int i, j, k;

  avgEvt = calloc(1, sizeof(Mario));
  memset(scratch, 0, 37 * 300 * sizeof(float));

  for (i = 0; i < x->numEvts; i++) {
    evt = x->rawEvts + i;
    for (j = 0; j < 37; j++) {
      for (k = 0; k < 300; k++) {
        scratch[j][k] += (float) evt->wf[j][k];
      }
    }
  }

  if (baselineFlag != 0) {
    for (i = 0; i < 37; i++) {
      avg = 0.;
      for (j = 0; j < 30; j++) {
        avg += scratch[i][j];
      }
      avg /= 30.;
      for (j = 0; j < 300; j++) {
        scratch[i][j] -= avg;
      }
    }
  }

  for (j = 0; j < 37; j++) {
    for (k = 0; k < 300; k++) {
      scratch[j][k] /= (float) x->numEvts;
      avgEvt->wf[j][k] = (short) scratch[j][k];
    }
  }

  for (i = 0; i < x->numEvts; i++) {
    evt = x->rawEvts;
    avgEvt->ccEnergy += evt->ccEnergy;
    for (j = 0; j < 36; j++) {
       avgEvt->segEnergy[j] += evt->segEnergy[j];
    }
  }
  avgEvt->ccEnergy /= (float) x->numEvts;
  for (i = 0; i < 36; i++) { avgEvt->segEnergy[i] /= (float) x->numEvts; }

  return avgEvt;
}

int main(int argc, char **argv) {

  FILE *fin, *fou, *ftr;
  struct gebData ghdr, defaulthdr = {100, sizeof(Mario), 0ll};
  Mario *evt;
  char evtlabel[] = {'a', 'b', 's'}, seglabel[] = {'n', 'l', 'r', 'u', 'd'};
  int i, j, k, num, seg, baselineFlag = 1;

  for (i = 0; i < 2; i++) {
    runList[i].rawEvts = malloc(MAX_EVT * sizeof(Mario));
  }

  for (i = 0; i < 2; i++) {
    fin = fopen(runList[i].filename, "r");
    if (fin == 0) { fprintf(stderr, "could not open file %s\n", runList[i].filename); exit(1);}
    while ((runList[i].numEvts < MAX_EVT) && (fread(&ghdr, sizeof(struct gebData), 1, fin) == 1)) {
      if (ghdr.type == 100) {
        num = fread(runList[i].rawEvts + runList[i].numEvts++, sizeof(Mario), 1, fin);
        assert(num == 1);
      }
      else {
        fseek(fin, ghdr.length, SEEK_CUR);
      }
    }
    fclose(fin);
  }

  pList[0].rawEvts = avg(runList + 0, baselineFlag);
  pList[1].rawEvts = avg(runList + 1, baselineFlag);

  fou = fopen("cevt.dat", "w");
  if (fou == 0) { fprintf(stderr, "could not open file cevt.dat\n"); exit(1);}
  for (i = 0; i < 2; i++) {
    assert( 1 == fwrite(&defaulthdr, sizeof(struct gebData), 1, fou));
    assert( 1 == fwrite(pList[i].rawEvts, sizeof(Mario), 1, fou));
  }
  fclose(fou);

  printf("list 1: %d evts, list 2: %d evts\n", runList[0].numEvts, runList[1].numEvts);

  ftr = fopen("tr.csv", "w");
  if (ftr == 0) { fprintf(stderr, "could not open file tr.csv\n"); exit(1);}
  fprintf(ftr, "evt, seg, ch, val\n");

  for (i = 0; i < 2; i++) {
    for (j = 0; j < 5; j++) {  // 'n', 'l', 'r', 'u', 'd'
        //seg = (j == 0) ? runList[i].seg : neigh[runList[i].seg][j - 1];
        //evt = runList[i].rawEvts + 7;  // take first evt
        seg = (j == 0) ? pList[i].seg : neigh[pList[i].seg][j - 1];
        evt = pList[i].rawEvts;  // take 1st evt
        for (k = 0; k < 300; k++) {
          fprintf(ftr, "%c,%c,%d,%d\n", evtlabel[i], seglabel[j], k + 1, evt->wf[seg][k]);
      }
    }
  }
  fclose(ftr);
  return 0;
}
