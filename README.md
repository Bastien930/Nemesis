# NEMESIS — Outil d'analyse et d'audit (usage autorisé)

NEMESIS est un outil en C permettant d’analyser des fichiers de type *shadow* et
d'exécuter des tests basés sur un dictionnaire dans un cadre légal : audit
interne, pentest autorisé, ou recherche en sécurité.

> **Avertissement légal**  
> L’utilisation est strictement limitée aux environnements pour lesquels vous
disposez d’une autorisation explicite.  
> L’auteur décline toute responsabilité en cas de mauvaise utilisation.

---

## Compilation (exclusivement via Makefile)

Le projet doit être compilé avec :

```bash
make install 
```
depuis le repertoire ./Nemesis/build


Le binaire généré apparaîtra dans le répertoire /usr/local/bin sous le nom :

```
nemesis
```

### Nettoyer les fichiers

```bash
make clean
```


---

## Installation de la page man

Une page man est fournie dans `docs/nemesis.1`.

### Installer la page man

```bash
sudo cp docs/nemesis.1 /usr/share/man/man1/
```

Puis mettre à jour le cache :

```bash
sudo mandb
```

### Vérifier l’installation

```bash
man nemesis
```

---

## Utilisation générale

```bash
nemesis [options]
```

Afficher l’aide :

```bash
nemesis --help
```

---

## Options principales

| Option | Description |
|--------|-------------|
| `-s, --shadow <file>` | Fichier shadow à analyser. |
| `-w, --wordlist <file>` | Wordlist pour mode dictionnaire. |
| `--resume` | Reprend depuis une sauvegarde. |
| `-m, --mangling <fast|balanced|agressive>` | Méthode de mangling. |
| `-d, --dictionary` | Mode dictionnaire. |
| `-b, --bruteforce` | Mode bruteforce. |
| `-c, --charset <preset|chars>` | Charset prédéfini ou personnalisé. |
| `--min <n>` | Longueur minimale. |
| `--max <n>` | Longueur maximale. |
| `-t, --threads <n>` | Nombre de threads. |
| `-o, --output <file>` | Fichier de sortie. |
| `--format <fmt>` | Format de sortie. |
| `--log <file>` | Logging vers fichier. |
| `-h, --help` | Afficher l’aide. |
| `--version` | Afficher la version. |

---

## Exemples

### Mode bruteforce

```bash
nemesis -s shadow.dump -b --charset "abcd" --output result.json --format json
```

### Mode dictionnaire

```bash
nemesis -s /etc/shadow -w rockyou.txt --mangling fast --log nemesis.log
```

### Reprise

```bash
nemesis --resume
```

---

## Structure

```
/src
  main.c
  core.c
  core.h
  utils.c
  utils.h
/include
  config.h
/docs
  nemesis.1
Makefile
README.md
```

---

## Auteur

**BASTIEN-ALEXIS-ILIAN**


