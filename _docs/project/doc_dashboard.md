# Dashboard Inferno — documentation

## Rôle

Interface graphique du projet. Elle se connecte au serveur, affiche les agents
connectés et permet de leur envoyer des commandes.

Le dashboard ne parle jamais directement aux agents. Tout passe par le serveur,
qui fait le relais dans les deux sens.

```
Dashboard  --COMMAND-->  Serveur  -->  Agent
                                        | exécute
Dashboard  <--RESPONSE--  Serveur  <----+
```

---

## Architecture

Deux parties séparées : le réseau et l'affichage. Elles ne se connaissent pas
directement, elles communiquent par des signaux Qt.

```
+-------------------- AFFICHAGE --------------------+
|  MainWindow                                       |
|     +-- AgentItemWidget      une ligne d'agent    |
|     +-- MetricCardsWidget    les 4 cartes         |
|     +-- LineChartWidget x4   les graphes          |
|     +-- ProcessTableWidget   le tableau process   |
+-------------------------+-------------------------+
                          | signaux Qt
+-------------------------+-------------------------+
|                      RÉSEAU                        |
|  ServerClient       connecte, écoute, trie         |
|  DashboardSession   buffer + découpage en frames   |
|  SocketFactory      choisit la socket selon l'OS   |
|  WindowsSocket / LinuxSocket                       |
+----------------------------------------------------+
```

`ServerClient` n'inclut aucun fichier de widget. Il envoie un signal quand il a
reçu quelque chose, et `MainWindow` décide quoi en faire.

---

## Les classes

| Classe | Rôle |
|---|---|
| `MainWindow` | Assemble les widgets et branche les boutons |
| `ServerClient` | Communication avec le serveur |
| `DashboardSession` | Buffer d'octets et découpage en frames |
| `AgentItemWidget` | Une ligne de la liste d'agents |
| `MetricCardsWidget` | Les 4 cartes du haut |
| `LineChartWidget` | Un graphe, dessiné avec QPainter |
| `ProcessTableWidget` | Le tableau RUNNING PROCESSES |
| `uiutils` | Fonction libre `makeLabel()` |
| `theme.h` | Les couleurs, nommées |

---

## Méthodes principales

### ServerClient

| Méthode | Reçoit | Rend |
|---|---|---|
| `connectToServer(host, port)` | `QString`, `quint16` | `bool` |
| `sendCommand(target, type, data)` | `QString`, `CommandType`, `QString` | `void` |
| `sendRegister()` *(privé)* | — | `void` |
| `onReadyRead()` *(privé)* | — | `void` |
| `handleFrame(frame)` *(privé)* | `const Frame&` | `void` |
| `handleData(payload)` *(privé)* | `const std::vector<uint8_t>&` | `void` |

**Signaux émis vers `MainWindow` :**

| Signal | Émis quand | Traité par |
|---|---|---|
| `agentReceived(id, name, details)` | un agent arrive | `addAgentItem()` |
| `responseReceived(target, text)` | une commande shell a répondu | `showOutput()` |
| `processListReceived(target, processes)` | la liste des process arrive | `setProcesses()` |

### MainWindow

| Méthode | Reçoit | Rend |
|---|---|---|
| `addAgentItem(id, name, details, online)` | 3 `QString` + `bool` | `void` |
| `showOutput(text)` | `const QString&` | `void` |
| `buildContentArea()` | — | `void` |
| `createChart(title, series, labels, ...)` | `QString`, `QVector<QVector<double>>`, `QStringList` | `LineChartWidget*` |
| `buildStatusBar()` | — | `void` |

Membres : `m_target` (MAC de l'agent sélectionné), `m_client`,
`m_processTable`, `m_metricCards`.

### ProcessTableWidget

| Méthode | Reçoit | Rend |
|---|---|---|
| `setProcesses(processes)` | `const std::vector<ProcessInfo>&` | `void` |
| `createSeparator()` *(privé)* | — | `QWidget*` |
| `createBar(value)` *(privé)* | `int` 0-100 | `QWidget*` |

### MetricCardsWidget

| Méthode | Reçoit | Rend |
|---|---|---|
| `updateMetric(key, value)` | `QString`, `QString` | `void` |
| `createMetricCard(key, title, value, subtitle)` *(privé)* | 4 `QString` | `QWidget*` |

### uiutils

| Fonction | Reçoit | Rend |
|---|---|---|
| `makeLabel(text, objectName)` | `QString`, `QString` | `QLabel*` |

---

## Choix techniques

**Les widgets sont gardés en pointeurs.** Avant j'écrivais
`insertWidget(0, new ProcessTableWidget(this))` : le widget s'affichait mais son
adresse était perdue. Maintenant je la garde dans `m_processTable` pour pouvoir
appeler `setProcesses()` quand les données arrivent.

**`setProcesses` vide puis reconstruit la grille.** Sinon les nouvelles lignes
s'empileraient sur les anciennes.

**Une struct d'affichage à part.** Le protocole donne des nombres, le tableau
veut du texte (`"18.3%"`, `"6.5 MB"`). D'où `ProcessRow`, distincte du
`ProcessInfo` du protocole — les deux portaient le même nom au début, ce qui
créait une collision.

**Une table `target → CommandType`.** Une frame `RESPONSE` ne dit pas ce qu'elle
contient, et l'id ne sert à rien puisque le dashboard envoie `id = 0` et que le
serveur assigne le vrai. Je note donc ce que j'ai demandé à chaque agent pour
savoir comment lire sa réponse.

**`makeLabel` est une fonction libre**, pas une méthode : elle ne garde aucun
état. Elle était copiée dans trois classes, il n'y en a plus qu'une.

---

## Du clic à l'affichage

Exemple avec le bouton **Process list** :

1. `MainWindow` appelle `sendCommand(m_target, RUNNING_PROCESSES, "")`
2. `ServerClient` construit et envoie une frame `COMMAND`
3. le serveur la transmet à l'agent, qui répond
4. le `QSocketNotifier` appelle `onReadyRead()`, qui sort la frame complète
5. `handleFrame()` voit une `RESPONSE` et retrouve le type de commande envoyée
6. il parse et émet `processListReceived`
7. `MainWindow` appelle `m_processTable->setProcesses()`
