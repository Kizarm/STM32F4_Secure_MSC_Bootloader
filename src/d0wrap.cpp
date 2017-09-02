#ifdef __arm__

/*
 * ABI je definováno takto:
 *<ctor-dtor-name>   ::= C1   # complete object constructor
 *                   ::= C2   # base object constructor
 *                   ::= C3   # complete object allocating constructor
 *                   ::= D0   # deleting destructor
 *                   ::= D1   # complete object destructor
 *                   ::= D2   # base object destructor
 *
 * <b>Jak zhruba funguje volání destruktorů, pokud používáme virtuální destruktory.</b>
 * 	Překladač vytvoří D1 a D0 destruktor a oba dá do VTABLE. Takže i když
 * 	objekt není vytvořen dynamicky, D0 nelze odstranit a musí být přítomen
 * 	operátor delete. Lze tedy definovat delete jako prázdnou funkci. Pokud
 * 	je použit někde jinde v programu, zhavaruje to protože nemá párové new.
 * Možná jde někde nastavit, aby se D0 nevytvářel, ale zatím to necháme tak.
 * */
void operator delete (void* p) {
  asm volatile ("bkpt 0");	// Chyba - tohle nemělo být voláno.
}

#endif // __ARM__
