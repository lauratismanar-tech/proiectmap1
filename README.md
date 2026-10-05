# Agenda de contacte

Proiect individual la disciplina Metode avansate de programare, anul universitar 2026-2027.

## Autor

- **Nume:** Tismanar Laura-Teodora
- **Grupa:** 2.2
- **Marca:** LH715713
- **Tema:** 1 - Agenda de contacte

## Descriere

Serviciu web care gestioneaza o agenda de contacte, asemanatoare cu cea din telefon. Permite adaugarea contactelor (nume, email, telefon, categorie), cautarea dupa nume, filtrarea pe categorie si stergerea lor, cu validarea datelor de intrare. Datele se pastreaza in memorie.

## Tehnologii

C++20 cu cpp-httplib si nlohmann/json

## Rulare

```
docker build -t map-proiect .
docker run -d -p 8080:8080 map-proiect
```

Aplicatia asculta pe portul 8080. Verificati:

```
curl http://localhost:8080/health
curl http://localhost:8080/version
```

## Testare

```
cmake -B build -DBUILD_TESTS=ON
cmake --build build -j
./build/tests
```

## Rutele implementate

| Ruta | Metoda | Descriere |
|---|---|---|
| `/health` | GET | Starea serviciului |
| `/version` | GET | Versiunea si commit-ul din care a fost construita imaginea |
| `/` | GET | Pagina de prezentare |
| `/reset` | POST | Goleste datele din memorie |
| `/contacts` | POST | Adauga un contact (name, email, phone, category) |
| `/contacts` | GET | Listeaza contactele, cu filtre optionale `category` si `q`, ordonate dupa name |
| `/contacts/{id}` | GET | Intoarce un contact sau 404 |
| `/contacts/{id}` | DELETE | Sterge un contact (204 la succes, 404 daca nu exista) |
| `/stats` | GET | Numarul total de contacte si numarul pe categorii |

## Decizii de implementare

1. **Email-ul se compara normalizat, dar se afiseaza asa cum a fost introdus.** Unicitatea se verifica pe forma cu litere mici, iar contactul pastreaza forma scrisa de utilizator. Alternativa era sa salvez direct forma normalizata, dar atunci utilizatorul ar vedea alt text decat a introdus.
2. **Datele se tin in memorie, intr-un container ordonat dupa id.** Id-urile sunt atribuite de server, incepand de la 1, iar `/reset` le reia de la 1. Alternativa era o baza de date, dar contractul cere stare in memorie.
3. **Validarea se face inainte de orice modificare a starii.** Un contact invalid (400) sau duplicat (409) nu schimba lista si nu consuma un id.