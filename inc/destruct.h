#ifndef DESTRUCTOR_H
#define DESTRUCTOR_H

#if DESTRUCTUSED
  #define haveDestructor(prm) prm
#else
  #define haveDestructor(prm)
#endif // SUPPRESSDESTRUCT

#endif // DESTRUCTOR_H
