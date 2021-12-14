#include "Drill.h"
#include "Arduino.h"

material_entry material_db[] = {
  // Speed in meters per minute
  { "Toolsteel",         10., },
  { "Stailness",         12., },
  { "Steel",             25., },
  { "Brass",             30., },
  { "Bronze",            30., },
  { "Cast iron",         15., },
  { "Copper",            20., },
  { "Aluminium",         60., },
  NULL,
};

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

// RPM = (1000 * surface speed) / (π * diameter)
// diameter = 1000 * surface speed [mm/min] /rpm [/min] / pi;

double diameter_mm2rpm(material_entry *e, double diam) {
  return 1000. * e->recc / M_PI / diam;
};

double rpm2diameter_mm(material_entry *e, double rpm) {
  return 1000. * e->recc / rpm / M_PI;
};
