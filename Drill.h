#ifndef _H_DRILLS
#define _H_DRILLS
typedef  struct {
  const char * name;
  double recc; // for HSS
} material_entry;

extern material_entry material_db[];

double diameter_mm2rpm(material_entry *e, double diam);
double rpm2diameter_mm(material_entry *e, double rpm);

#endif
