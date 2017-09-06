#ifndef CONTEXTSERVER_H
#define CONTEXTSERVER_H

/**
 * @class ContextServer
 * Poskytuje obsah firmware v šifrované podobě na základě obsahu požadavku v cestě.
 * 
 * Průběh je jednoduchý:
 * - 1. Cesta se odseparuje pomocí scanf() do proměnné cuid, což je UUID jednoduše zašifrované (XOR) pomocí SecretKeys.server.
 * - 2. XOR vytvoří čisté UUID, to se kontroluje na oprávnění (zde jen seznam oprávněných payed).
 * - 3. XOR SecretKeys.loader s UUID vytvoří klíč GeneratedKeys.loader k šifrování obsahu.
 * - 4. Zkontroluje se zdrojový firmware
 *      + a. Má správný Magic number ?
 *      + b. Doplní se délka, okrouhle na 8 nahoru (pro šifrování)
 *      + c. Doplní se kontrolní součet
 * - 5. Zdrojový firmware se zašifruje a odešle klientovi.
 * */

class ContextServer {
  public:
    ContextServer (const char * r);
    ~ContextServer();
    /// Hlavní metoda serveru
    bool path (int sockfd, char * rq);
  protected:
    /// Pomocné metody
    bool sendImage(int sockfd, const char * name);
    bool sendFile (int sockfd, const char * name);
    bool sendFw   (int sockfd, const char * name);
    bool sendSel  (int sockfd);
    void notFound (int sockfd);
  private:
    const char * root;
};

#endif // CONTEXTSERVER_H
