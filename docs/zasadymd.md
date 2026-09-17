# Zasady Aktualizacji Plikow `.md`

Ten plik opisuje zasady, ktore stosujemy przy aktualizacji dokumentacji projektu w plikach `.md`.

> **Rewizja 2026-09-13:** stara, płaska skala zielony/żółty/czerwony zastąpiona
> siedmiostopniową drabinką dojrzałości/postępu (pomysł → do rozważenia →
> plan → zaczęte → w toku wczesny/zaawansowany → ukończone) plus osobnym
> czerwonym na błędy/zagrożenia (patrz §1-§2 niżej). Powód: płaski
> "żółty = wszystko co nie jest gotowe ani zagrożeniem" nie odróżniał
> świeżo zaczętego szkieletu od czegoś tuż przed ukończeniem — dokładnie
> ten rodzaj rozjazdu faktów, który te zasady mają eliminować. Historyczne
> wpisy sprzed tej daty (setki `color: green` w całym `docs/`) **NIE są
> retroaktywnie przemalowywane** — zbyt duże ryzyko mechanicznej edycji 70
> plików za znikomą korzyść; zwykły `green` sprzed rewizji czytaj jako
> dzisiejszą "butelkową zieleń" (ukończone). Nowe wpisy, od tej daty, mają
> używać dokładnej palety z §1.

## 1. Pełna skala: od pomysłu do ukończenia

Zamiast jednego "żółtego" na wszystko co nie jest ani gotowe, ani
zagrożeniem, używamy drabinki dojrzałości + postępu — od surowego pomysłu
do ukończenia:

| # | Etap | Kolor | Kod | Znaczenie |
|---|---|---|---|---|
| 1 | Pomysł | 🟣 fioletowy | `purple` | Surowy pomysł, nieoceniony — jeszcze nie zdecydowano, czy w ogóle wchodzi w zakres projektu |
| 2 | Do rozważenia | 🔷 turkusowy | `#17A2B8` | W ocenie — pomysł uznano za sensowny, waży się szczegóły/podejście, jeszcze niezdecydowane |
| 3 | Zaakceptowany plan | ⬜ szary | `gray` | Zdecydowane, zero roboty — uznana, śledzona pozycja (np. wiersz w tabeli), ale bez startu prac |
| 4 | Zaczęte | 🟡 żółty | `yellow` | Pierwsze kroki, sam szkielet, nic jeszcze realnie nie działa |
| 5 | W toku (wczesny etap) | 🟠 pomarańczowy | `orange` | Część działa, większość jeszcze brakuje |
| 6 | W toku (zaawansowany etap) | 🔵 granatowy | `navy` | Większość gotowa, trwa dopinanie szczegółów/edge case'ów |
| 7 | Ukończone | 🟢 butelkowa zieleń | `#006A4E` | Gotowe |

> **Uwaga o kolizji:** `#17A2B8` (turkusowy) i `navy` (#000080) zostały
> dobrane świadomie tak, by NIE kolidować z lokalną paletą kategorii błędów
> używaną w `Daraja_audyt_1.md`/`Nexa_audyt_1.md`/`Sayari_UI_audyt_1.md`
> (tam `#0E7490` = kategoria "Wydajność", `#003366` = "częściowo otwarte" —
> inna oś: waga błędu, nie postęp prac). `#006A4E` (butelkowa zieleń) jest
> celowo tym samym kodem co "naprawione" w tamtej palecie — tu i tam znaczy
> to samo, więc nie trzeba dwóch odcieni zieleni.

Przy KAŻDYM z tych wpisów ma pojawić się dopisek nazywający etap wprost
(`nowy pomysł`, `do rozważenia`, `plan`, `zaczęte`, `w toku (wczesny etap)`,
`w toku (zaawansowany etap)`, `wykonane`) oraz data (§2 niżej). Rzeczy
ukończone **nie mają być usuwane** z plików `.md` — zostają w dokumentacji
jako historia stanu projektu.

Przykłady:

```md
<span style="color: purple;">propozycja: dedykowany format snapshotów AYFS2 nowy pomysł 2026-09-13</span>
<span style="color: #17A2B8;">snapshoty AYFS2: format CoW vs redo-log do rozważenia 2026-09-13</span>
<span style="color: gray;">parser journala AYJ3 plan 2026-09-13</span>
<span style="color: yellow;">szkielet alokatora klastrowego zaczęte 2026-09-13</span>
<span style="color: orange;">alokator klastrowy: alokacja działa, brak jeszcze free() w toku (wczesny etap) 2026-09-13</span>
<span style="color: navy;">alokator klastrowy: alokacja+free gotowe, dopinane edge case'y sparse w toku (zaawansowany etap) 2026-09-13</span>
<span style="color: #006A4E;">alokator klastrowy bigalloc RW wykonane 2026-09-13</span>
```

## 2. Błędy i zagrożenia (czerwień)

- rzeczy będące realnym zagrożeniem architektonicznym, blokerem dla
  dalszego rozwoju, albo znanym, jeszcze nienaprawionym błędem, mają być
  oznaczane na czerwono — niezależnie od tego, na którym z powyższych
  etapów (§1) akurat się znajdują
- przy takich wpisach ma pojawic sie dopisek `błąd` albo `zagrożenie`
- tej kategorii nie nalezy naduzywac; ma byc zarezerwowana dla rzeczy, ktore
  realnie blokuja, podwazaja dalszy rozwoj systemu, albo psuja dzialanie

Przyklad:

```md
<span style="color: red;">opis blokera architektonicznego zagrożenie 2026-09-13</span>
<span style="color: red;">race warunek w Fsck przy 2 rownoleglych operacjach błąd 2026-09-13</span>
```

## 3. Data przy kazdym wpisie

- **KAZDORAZOWO** przy dodaniu lub aktualizacji wpisu nalezy dodac aktualna date
- data powinna byc w formacie `YYYY-MM-DD` (np. `2026-05-17`)
- data powinna pojawic sie przy opisie (na koncu linijki lub w osobnym wierszu)
- dotyczy to WSZYSTKICH kolorów z §1-§2 bez wyjątku

## 4. Wzajemne odnośniki między powiązanymi plikami

- jeśli plik ma "rodzeństwo" (starsza/nowsza runda tego samego audytu, plan
  vs. status, audyt vs. kontrakt, wersja archiwalna) — na górze obu plików
  **musi** pojawić się jawna linia z linkiem do drugiego, np.
  `**Powiązane:** [nazwa_pliku.md](nazwa_pliku.md) — jednym zdaniem czym się różnią`
- to nie jest sugestia stylistyczna: bez tego czytelnik (człowiek albo AI)
  nie ma jak się dowiedzieć, że istnieje inna wersja tych samych faktów, i
  aktualizuje tylko jeden plik — dokładnie tak fakty się rozjeżdżają
- dobre istniejące przykłady tego wzorca: `ext234_audit.md` ↔
  `ext234_full_contract.md`, `zfs_impl_plan.md` ↔ `zfs_compat.md`

## 5. Jedno źródło prawdy na fakt

- jeśli jakiś mierzalny fakt (procent ukończenia, liczba PASS/FAIL, status
  warstwy Core/Common/Edge/Exotic) żyje w tabeli zbiorczej (np.
  `fs_for_AyoOS.md`), plik szczegółowy (audyt/kontrakt) ma do tej tabeli
  **linkować**, a nie powtarzać tę samą liczbę osobnym zdaniem
- wyjątek: prozaiczne uzasadnienie/historia DOJŚCIA do liczby może i powinno
  żyć w pliku szczegółowym — tylko sama aktualna liczba ma mieć jedno miejsce
- przy każdej aktualizacji faktu: zaktualizuj TYLKO źródło prawdy; plik
  odsyłający nie wymaga zmiany, bo i tak tylko linkuje

## 6. Nagłówek "ostatnia weryfikacja całości"

- każdy plik `.md` w `docs/` ma mieć na samej górze (pod tytułem, przed
  pierwszym akapitem) linię: `**Ostatnia weryfikacja całości:** YYYY-MM-DD`
- to jest odrębne od dat przy pojedynczych wpisach (§3) — tamte mówią kiedy
  PUNKT powstał, ta linia mówi kiedy ktoś ostatnio przeszedł CAŁY plik i
  potwierdził że nadal jest spójny z rzeczywistością
- pozwala na pierwszy rzut oka wyłapać plik, który nikt nie dotykał od
  tygodni, zanim ktoś zacznie mu ufać jako aktualnemu

## 7. Nazewnictwo plików

- zakaz niejednoznacznych sufiksów w nazwach plików: `(1)`, `(2)`, `_v2`,
  `_final`, `_new`, `_kopia` i podobnych — nazwa pliku ma od razu mówić,
  czym plik jest, bez otwierania go
- właściwe sufiksy: `_audyt`/`_audit` (raport z przeglądu), `_plan`
  (zamierzenia na przyszłość), `_compat` (status zgodności/kontrakt),
  `_archiwum` (świadomie zarchiwizowane, nieaktualizowane dalej), data
  `_2026-MM-DD` (migawka konkretnego dnia, gdy kolejne rundy mają zostać
  odrębnymi plikami, jak audyty AYFS2)
- jeśli plik dostaje następcę, stary dostaje `_archiwum` (albo trafia do
  §4-owego odnośnika jako "wersja historyczna") zamiast zostawać z mylącą,
  niejasną nazwą

## 8. Indeks dokumentacji

- `docs/README.md` ma być krótką tabelą "temat → plik(i)", zaktualizowaną
  przy każdym dodaniu/scaleniu/archiwizacji pliku w `docs/`
- to jest pierwsze miejsce, w które zagląda się przy pytaniu "gdzie żyje
  dokumentacja X", zamiast grepowania kilkudziesięciu plików

## 9. Cel tych zasad

- zachowanie historii tego, co zostalo juz dowiezione
- czytelne odroznienie pelnej drabinki dojrzalosci (fioletowy→turkusowy→
  szary→żółty→pomarańczowy→granatowy→butelkowa zieleń) od bledow/zagrozen
  (czerwony)
- jedno źródło prawdy na fakt, żeby liczby/statusy w różnych plikach nie
  rozjeżdżały się z czasem
- łatwe zorientowanie się, który plik jest aktualny, a który zdezaktualizowany


## Rejestr Zmian i Kopii Zapasowych AI — zarchiwizowane

Ta sekcja (ręczny log "przed każdą zmianą zapisz oryginalny kod", przeniesiony
niegdyś z `AI_BACKUP.md`) została wydzielona do osobnego pliku:
**[`history/ai_rollback_memory_archiwum.md`](history/ai_rollback_memory_archiwum.md)**.

<span style="color: red;">Praktyka zarzucona — ostatni realny wpis 2026-07-28,
zero wpisów przez kolejne ~1,5 miesiąca aktywnego rozwoju. Zastąpiona przez
`git` (commit/diff/revert na każdą zmianę) jako jedyny, pełniejszy mechanizm
cofania zmian zagrożenie 2026-09-13</span> — patrz nota na górze pliku
archiwalnego po pełne uzasadnienie. Nowe wpisy tego typu **nie mają już
powstawać**; to nie jest aktywna praktyka projektu.
