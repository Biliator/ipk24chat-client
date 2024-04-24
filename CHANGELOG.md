### chyby
* nevím přesně, zda EOF se zaznamená a správně zpracuje, nezvládl jsem to přesně otestovat
* nešlo makenout na Merlinovi, pouze na Virtualce IPK24.ova
* u TCP jsem zapomněl ošetřit případ, kdy klient pošle např. join a poté rychle MSG
  ještě dřív, než dojde REPLY, což způsobí exit aplikace