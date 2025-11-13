#include <stdio.h>
#include <stdlib.h>

#include <CCFG.h>

int main() {
  CCFG_Object* val = parseFile("test.ccfg");

  printObject(val);

  CCFG_Composite comp = getComposite(val);
  if (comp.toTable == nullptr)
    return 1;
  
  CCFG_SimpleTable* tbl = comp.toTable;

  CCFG_SimpleTable* inside = stget(tbl, "server").composite.toTable;

  return 0;
}
