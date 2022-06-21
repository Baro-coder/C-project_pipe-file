#C-project_pipe-file

## Opis

Zestaw programów typu 'producent', 'konsument' prezentujący możliwości synchronizacji i współbieżnego działania kilku procesów zdolnych do obróbki i przekazywania sobie danych.

___

**System operacyjny:** Linux
**Język:** C
**Mechanizmy synchronizacji:**
- łącza nienazwane (pipe),
- plik (file)

## Uruchomienie projektu:
Przejście do katalogu projektu:
> cd *existing project folder*

Wywołanie pliku uruchamiającego:
> ./build_and_run.sh

Skrypt uruchamiający samodzielnie kompiluje kod źródłowy i w sytuacji powodzenia uruchamia program.

## Procesy
### Proces potomny 1:
Czyta dane  z  stdin i przekazuje je
do procesu 2 w niezmienionej formie
przez łącze nienazwane (pipe).

### Proces potomny 2:
Pobiera  dane  od procesu 1  i  konwertuje
je do postaci heksadecymalnej.  Następnie
przekazuje do procesu 3 w pliku wynikowym.

### Proces potomny 3:
Pobiera  dane z pliku zapisanego przez
proces 2  i  wypisuje  je  na stderr. 
Jednostki  danych  wyprowadzone po 15 
w  pojedynczym  wierszu  i oddzielone
spacjami.
         
___

Wszystkie 3 procesy są powoływane z jednego procesu
inicjującego. Po powołaniu proces inicjujący wstrzymuje
pracę. Wznawia proces po zakończeniu działania procesów
potomnych i 'sprząta' przed zakończeniem działania.

## Sygnały

Operator może wysłać do dowolnego procesu sygnały:
- S1 - SIGINT  - zakończenie działania
- S2 - SIGUSR1 - wstrzymanie działania
- S3 - SIGUSR2 - wznowienie  działania

Proces odbierający  sygnał  musi powiadomić pozostałe procesy
o otrzymanym sygnale.  Wówczas wszystkie procesy realizują
odpowiednie operacje będące interpretacją odebranego sygnału.
