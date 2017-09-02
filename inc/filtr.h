typedef int real;
/// Přenos je 1 / (1 + w/w0) -> RC článek, w0 je zlomová frekvence
class Filtr {
  public:
    Filtr() {
      y1 = 0;
      real k = 20; // Zlomová frekvence je fs / k =~ 2 kHz
      b0 = (1 << 16) / (k + 1);
      a1 = b0 * k;
    };
    real run (real x0) {
      real y0 = (b0 * x0) + (a1 * y1);
      y0 >>= 16;   // Normalizuj
      y1   = y0; x1 = x0;
      return y0;
    };
  private:
    real x1, y1;
    real b0, a1;
};
