#ifndef EXTI0_H
#define EXTI0_H
/// Softwarové přerušení na nízké prioritě.
/// Není ekvivalentní volání podprogramu (funkce).
/// Pokud je to voláno z přerušení o vyšší prioritě, provede se toto celé a až pak následně
/// to softwarové (které může být i nadále přerušováno). Tedy odložená rutina, která probíhá
/// na pozadí (mimo hlavní smyčku) a nemusí být hotova ihned.
/// Kód je navržen jen pro toto jediné přerušení, jeho obsluha je mimo třídu !
class Exti0 {
  public:
    /// Konfigurace se nijak nedotýká připojeného GPIO pinu (Knihovna ST to neumí).
    void Config (void) {
    };
    /// Vyvolej přerušení
    void Intr (void);
    /// Nuluj příznak přerušení.
    void Clr (void) {
    };
    /// Povolí přerušení od EXTI0
    void Start(void) {
    };
    /// Zakáže přerušení od EXTI0
    void Stop (void) {
    };
};

#endif // EXTI0_H
