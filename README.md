# IPK chatovací aplikace

## 1. Úvod
IPK chatovací aplikace umožňuje posílat a přijímat uživateli zprávy.
Uživatel začíná přihlášením a po úspěšné autorizaci, je připojen do
výchozího kanálu, kde odtud může komunikovat s dalšími účastníky. Program byl vyvíjen ve WSL a ne všechny knihovny musí být na jiných distribucí Linuxu dostupné. Např. na Merlinovi mi nešel Makefile, ale na Virtuálce, která byla dodáná k projektu, mi to šlo.

### make a spuštění
krok 1.
```
make
```

krok 2.
```
./ipk24chat-client -s IPv4 -p PORT -t [udp|tcp] -d [TIME IN MS] -r [NUMBER] -h
```
-d a -r nepovinný a pouze u udp
-h je nápověda

## 2. Teorie
V této části budou vysvětleny základní principy a koncepty, které jsou nezbytné pro porozumění fungování aplikace.

### UDP
UDP je zkratka pro User Datagram Protocol a je to jednoduchý, bezspojkový, nespolehlivý protokol vrstvy transportu v síťové architektuře TCP/IP. UDP přenáší datagramy nezávisle na sobě, což znamená, že každý datagram může cestovat jinou cestou a dorazit v jiném pořadí než byl odeslán. Nezajišťuje žádnou formu potvrzení doručení nebo opakování přenosu dat, což znamená, že není zaručeno, že data dorazí v pořádku, ani že dorazí vůbec.

### TCP
TCP je zkratka pro Transmission Control Protocol a je to spojově orientovaný, spolehlivý protokol vrstvy transportu v síťové architektuře TCP/IP. TCP zajišťuje spolehlivý přenos dat tím, že kontroluje přenos dat mezi odesílatelem a příjemcem, potvrzuje doručení dat a řeší problémy, jako jsou ztráty paketů, opoždění a duplicity. Pro dosažení spolehlivého doručení TCP využívá mechanismy jako je potvrzování doručení, opakování přenosu, řízení toku a řízení zahlcení.

### ABNF
ABNF je notace používaná pro popis syntaxe formálních jazyků. Je to rozšířená verze Backus-Naur form (BNF), která poskytuje možnost specifikovat gramatiku jazyka pomocí pravidel a symbolů.

Příklad:
```
message = greeting 1*(word " ") "!"
greeting = "Hello" | "Hi" | "Hey"
word = 1*ALPHA
```

ABNF poskytuje jednoduchý a přehledný způsob popisu syntaxe jazyka, což usnadňuje pochopení a implementaci protokolů a formálních jazyků.

## 3. Architektura
Po zkontrolování argumentu zadané v konzoli a rozhodnutí o protokolu, se
vykoná buď funkce `udp` nebo `tcp`. Obě funkce fungují na podobném principu.
Začíná se v počátečním stavu (`START` pro udp, `1` pro TCP), pomocí `poll` se získá buď vstup od uživatele a ten se pošle, nebo odpověď od serveru, který se zpracuje. Poté se rozhodne o následujícím stavu.

### uživatelův vstup
Funkce `poll` zachytí vstup z konzole. Pomocí funkce check_input zkontroluje zda nebyl zadán příkaz (`/auth`, `/join`, `/rename`, `/help`). Pokud je to příkaz, provede ho a naplní proměnou buff řetězcem, který je pak odeslán serveru ke zpracování.

### zprávy od serveru
Funkce `poll` zachytí signál ze soketu. Přejde do switche a podle toho, co přišlo ze serveru, tak rozhodne o postupu.

### Rozdíly
TCP a UDP je ale zpracováváno jinak. Každý jinak kontroluje a pracuje se vstupy od uživatele a odpovědí od serveru.

#### TCP
Po zpracování argumentů, je nastaveno tcp spojení. Pro to existuje funkce tcp, která byla převzata z NESFIT, IPK projekty. Ve funkci je vytvořen a na bindován soket a následně poslouchá.

Tcp má 5 stavů:
* `1` - začátek, slouží pro zaslání `AUTH` zprávy
* `2` - tady lze zasílat `MSG`, `JOIN` serveru a přijímat `MSG`, `REPLY` od serveru
* `3` - pokud je odpověď `ERR`, nebo nesmyslná zpráva, přejde se do tohohle stavu
* `4` - ukončovací stav, do kterého se přejde po zaslání `BYE`
* `5` - stav po zadání příkazu `/join`

Zároveň ve všech stavech lze zaslat/přijmout `ERR` a `BYE`, po kterém dojde k ukončení programu.
Funkce `check_response` vezme odpověď od serveru, prochází slovo po slovu a kontroluje, zda se shoduje odpověď s nějakou gramatikou zpráv. (MSG, REPLY, atd.)

Další část pro tcp se nachází v `tcp.c/tcp.h`. Funkce začínající slovem content setaví zprávu a přiřadí jí pointeru buff. A funkce začínající tcp_check kontrolují korektnost příchozích zpráv.

#### UDP
TCP má 12 stavu (enum `State`), začínající stavem START. Každá klientova akce (odeslání `MSG`, odeslání `ATUH`) má 2 a 3 stavy. Po odeslání, se přejde do stavu, kde se čeká na `CONFIRM`. Pokud rozpracovaná akce je AUTH, tak se ještě přejde do stavu, kde se čeká na REPLY a pak buď pokračuje dalším stavem, nebo vrátí do původního stavu.

```
START (send AUTH) -> AUTH_SEND (wait CONFIRM) -> AUTH_CONF (wait REPLY|send CONFIRM) -> MSG_SEND/START

MSG_SEND (send MSG) -> MSG_CONF (wait CONFIRM) -> MSG_SEND
```

U přijímání zpráv dojde k okamžitému odeslání `CONFIRM`. Pouze pokud zpráva nebyla už předtím zpracování, tak se zpracuje. K tomu slouží struktura Node v `udp_id_history.c/udp_id_history.h`. Podobná, ale trochu jíná struktura ipk_list v udp_fifo.c/udp_fifo.h, slouží pro ukládání uživatelova vstupu. Uživatel musí čekat na zpracování zprávy, pokud však zadá rychle mnoho zpráv, pokud by CONFIRM dorazil později, tak by zprávy byly ztraceny a k tomu slouží fifo. Všechen input se ukládá do fifo a až je zpráva potvrzena druhou stranou, tak pak je z fifo odendání záznam a zpracování.


Další část pro udp se nachází v `udp.c/udp.h`. Obsahuje funkce pro sestavení zpráv (bye, auth, atd.) a přiřadí je pointeru buff. `message_id_increase` slouží pro inkrementaci ID zpráv. `udp_message_next` je pomocná funkce pro kontrolu korektnosti zpráv a získání informací z ní.


## 4. Testování
Tato část dokumentace obsahuje informace o testování aplikace. Zahrnuje popis testovací metodiky, prostředí testování, provedené testy a výsledky.


### TCP testování
Pro tcp bylo využít `netcat`. Na jedné konzoli běžel lokálně server pomocí netcat a druhá simulovala klienta. Zahájení komunikace začalo: 
```
./ipk24chat-client -s localhost -p 4567 -t tcp
```
a server začal a zaznamenal připojení:

```
nc -4 -l -C -v 127.0.0.1 4567
Listening on localhost 4567
Connection received on localhost port
```

#### test 1:
- testuje záklání komunikaci bez chyby
- základní připojení, poslání zprávy "HI" a ukončení

klient:
```  
/auth aaa bbb ccc
Success: nice
HI
^C
```

server
```
AUTH aaa AS ccc USING bbb
REPLY OK IS nice
MSG FROM ccc IS HI
BYE
```

#### test 2:
- testuje záklání komunikaci se špatným AUTH
- základní připojení

klient:
```  
/auth aaa bbb ccc
Failure: wrong
^C
```

server
```
AUTH aaa AS ccc USING bbb
REPLY NOK IS wrong
BYE
```

#### test 3:
- klient zašle "HI"
- klientovi dorazí ERR
- testuje kontrolu ERR a MSG

klient:
```  
/auth aaa bbb ccc
Success: nice
HI
ERR FROM Server: oop!
```

server
```
AUTH aaa AS ccc USING bbb
REPLY OK IS nice
MSG FROM ccc IS HI
ERR FROM Server IS oop!
BYE
```

#### test 4:
- klient zašle "HI"
- klientovi dorazí "HELLO"
- klient ukončí komunikaci
- testuje vzájemnou komunikaci bez chyb

klient:
```  
/auth aaa bbb ccc
Success: nice
HI
xxx: HELLO
^C
```

server
```
AUTH aaa AS ccc USING bbb
REPLY OK IS nice
MSG FROM ccc IS HI
MSG FROM xxx IS HELLO
BYE
```

#### test 5
- pošle HI a odpoví se mu nezmámý formát
- testuje reakci na nesmysl

klient:
```  
/auth aaa bbb ccc
Success: nice
HI
ERR: Unrecognized message from server!
```

server
```
AUTH aaa AS ccc USING bbb
REPLY OK IS nice
MSG FROM ccc IS HI
NESMYSL
ERR FROM ccc IS Unrecognized message from server!
BYE
```

#### test 6
- testuje, zda server může ukončit spojení pomocí BYE

klient:
```  
/auth aaa bbb ccc
Success: nice
```

server
```
AUTH aaa AS ccc USING bbb
REPLY OK IS nice
BYE
```

### UDP testování
Pro udp jsem využíval program Packet Sender. Timeout byl vždy 10 sekund, abych mohl stihnout odpovědět přes Packet Sender. 
Navazání spojení se provedlo: ```./ipk24chat-client -s IPv4 -p port -t udp -d 10000 -r 3```. Pomocí K reprezentují klienta a pomocí S server.

#### test 1:
- testuje záklání komunikaci bez chyby
- základní připojení, poslání zprávy "HI" a ukončení
  
klient
```
/auth aaa bbb ccc
Success: Success
HI
```

```
K: \02\00\00aaa\00ccc\00bbb\00
S: \00\00\00
S: \01\00\00\01\00\00Success\00
K: \00\00\00
K: \04\00\01ccc\00HI\00
S: \00\00\01
K: \ff\00\02
S: \00\00\02
```

#### test 2:
- testuje záklání komunikaci se špatným AUTH
- základní připojení

klient:
```  
/auth aaa bbb ccc
Failure: Failure
^C
```

server
```
K: \02\00\00aaa\00ccc\00bbb\00
S: \00\00\00
S: \01\00\00\00\00\00Failure\00
K: \00\00\00
S: \ff\00\01
K: \00\00\01
```

#### test 3:
- klient zašle "HI"
- klientovi dorazí ERR
- testuje kontrolu ERR a MSG

klient:
```  
/auth aaa bbb ccc
Success: Success
HI
ERR FROM Server: ERROR
```

server
```
K: \02\00\00aaa\00ccc\00bbb\00
S: \00\00\00
S: \01\00\00\01\00\00Success\00
K: \00\00\00
K: \04\00\01ccc\00HI\00
S: \00\00\01
S: \fe\00\02Server\00ERROR\00
K: \00\00\02
k: \ff\00\02
S: \00\00\02
```

#### test 4:
- klient zašle "HI"
- klientovi dorazí "HELLO"
- klient ukončí komunikaci
- testuje vzájemnou komunikaci bez chyb

klient:
```  
/auth aaa bbb ccc
Success: Success
HI
Server: HELLO
^C
```

server
```
K: \02\00\00aaa\00ccc\00bbb\00
S: \00\00\00
S: \01\00\00\01\00\00Success\00
K: \00\00\00
K: \04\00\01ccc\00HI\00
S: \00\00\01
S: \04\00\01Server\00HELLO\00
K: \00\00\01
K: \ff\00\02
S: \00\00\02
```

#### test 5
- pošle HI a odpoví se mu nezmámý formát
- testuje reakci na nesmysl

klient:
```  
/auth aaa bbb ccc
Success: Success
HI
ERR: Uknown message!
```

server
```
K: \02\00\00aaa\00ccc\00bbb\00
S: \00\00\00
S: \01\00\00\01\00\00Success\00
K: \00\00\00
K: \04\00\01ccc\00HI\00
S: \00\00\01
S: \n\ba\ba\ba\ab
K: \00\ba\ba
K: \fe\00\02ccc\00Uknown message!\n\00
S: \00\00\02
K: \ff\00\03
S: \00\00\03
```

#### test 6
- testuje, zda server může ukončit spojení pomocí BYE

klient:
```  
/auth aaa bbb ccc
Success: Success
```

server
```
K: \02\00\00aaa\00ccc\00bbb\00
S: \00\00\00
S: \01\00\00\01\00\00Success\00
K: \00\00\00
S: \ff\00\01
K: \00\00\01
```

#### test 7
- testuje, zda odešle zprávu více krát, pokud nepříjde CONFIRM včas
- zpráva se přeposlal každých 10 sekund a celkem se poslala 2x a na po třetí dorazil CONFIRM

klient:
```  
/auth aaa bbb ccc
Success: Success
HI
^C
```

server
```
K: \02\00\00aaa\00ccc\00bbb\00
S: \00\00\00
S: \01\00\00\01\00\00Success\00
K: \00\00\00
K: \04\00\01ccc\00HI\00
K: \04\00\01ccc\00HI\00
S: \00\00\01
K: \ff\00\02
S: \00\00\02
```

#### test 8
- test, zda nastane timeou, pokud nepříjde CONFIRM vůbec
- zpráva se přeposlal každých 10 sekund a celkem se poslala 3x

klient:
```  
/auth aaa bbb ccc
Success: Success
HI
ERR: Timeout and retransmition failed!
```

server
```
K: \02\00\00aaa\00ccc\00bbb\00
S: \00\00\00
S: \01\00\00\01\00\00Success\00
K: \00\00\00
K: \04\00\01ccc\00HI\00     19:16:45
K: \04\00\01ccc\00HI\00     19:16:55
K: \04\00\01ccc\00HI\00     19:17:05
```

#### test 9
- program nezpracuje zprávu, kterou už předtím zpracoval

klient:
```  
/auth a b c
Success: Success
Server: Joined default.
```

server
```
K: \02\00\00aaa\00ccc\00bbb\00
S: \00\00\00
S: \01\00\00\01\00\00Success\00
K: \00\00\00
S: \04\00\01Server\00Joined default.\00
K: \00\00\01
S: \04\00\01Server\00Joined default.\00
K: \00\00\01
K: \ff\00\01
S: \00\00\01
```

## Bibliografie

User Datagram Protocol. (srpen, 1980). J. Postel.
https://datatracker.ietf.org/doc/html/rfc768

Datatracker, Transmission Control Protocol (TCP). (srpen, 2022). Wesley Eddy.
https://datatracker.ietf.org/doc/html/rfc9293

Datatracker, Augmented BNF for Syntax Specifications: ABNF. (leden, 2008). Dave Crocker, Paul Overell.
https://datatracker.ietf.org/doc/html/rfc5234

Beej's Guide to Network Programming. (8. dubna, 2023). Brian “Beej Jorgensen” Hall.
https://beej.us/guide/bgnet/html/

NESFIT/IPK-Projekty. (2024).
https://git.fit.vutbr.cz/NESFIT/IPK-Projects-2024/src/branch/master/Stubs/cpp

The GNU Netcat project. (1.listopadu 2006). Giovanni Giacobbi.
https://netcat.sourceforge.net/