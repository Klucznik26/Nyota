# Host AyoOS

AyoOS jest pierwszym systemem, na którym działa Nyota.
Edytor Tunga to **osobny projekt** — nie kopiuje się tutaj.

Interpreter w AyoOS: na razie kopia `nyota.c`. Celem jest jeden `src/nyota.c`.

I/O idzie przez `NyotaHost` (`NyotaSetHost`). AyoOS podłącza AyoAPI jako backend
hosta, nie jako język. Tunga (osobny edytor) ma wołać `NyotaSetHost` + `NyotaEmbedRun`.
