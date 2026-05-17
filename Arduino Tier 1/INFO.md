1. 📟 Kod dla Arduino (jako mostek Serial -> PPM)

Ta wersja jest idealna, jeśli chcesz używać standardowego odbiornika RC, który akceptuje sygnał PPM. Arduino będzie odbierać dane o 8 kanałach z aplikacji PC przez port szeregowy i konwertować je na jeden, zbiorczy sygnał PPM.

Kluczowe ulepszenia:

Obsługa 8 kanałów: Kod jest zgodny z aplikacją PC, która wysyła dane dla 8 kanałów.

Zwiększona prędkość: Baudrate podniesiony do 115200 dla szybszej i bardziej niezawodnej komunikacji.

Failsafe: Jeśli w ciągu 0.5 sekundy nie nadejdą nowe dane, Arduino automatycznie ustawi wszystkie kanały na pozycję neutralną (1500µs), zapobiegając niekontrolowanemu zachowaniu modelu.

Wskaźnik LED: Wbudowana dioda LED informuje o statusie połączenia.

Jak to działa:

W aplikacji PC wybierz tryb "Arduino (Serial)".

Wybierz odpowiedni port COM i baudrate 115200.

Połącz się. Aplikacja zacznie wysyłać dane w formacie 1500,1500,1500,...\n.

Arduino odbierze te dane, przetworzy je i wygeneruje sygnał PPM na pinie D10.
