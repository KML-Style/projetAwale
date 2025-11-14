# Projet Awalé

Ce projet implémente un jeu d'Awalé en réseau avec serveur et clients.

## Compilation

Pour compiler le projet, allez dans le répertoire du projet et tapez :

```bash
make
```
## Lancement

Pour lancer le serveur,

```bash
./bin/serveur
```

Pour lancer un client une fois le serveur démarré,
```bash
./bin/client 127.0.0.1 8080
```

## Fonctionnalitées

### Fonctionnalitées principales

- Jouer à l’Awalé en réseau contre un autre joueur.

- Mode observateur pour suivre une partie en cours.

- Sauvegarde des parties dans des fichiers .txt.

- Envoyer des messages privés et des messages géneraux sur le tchat.

### Fonctionnalitées annexes 

- Ajouter/supprimer des amis.

- Possibilité de désactiver le suivi de ses parties pour les utilisateurs non-amis.

- Afficher une biographie de joueur.

## Liste des commandes

- LIST : voir les joueurs connectés disponibles

- GAMES / LIST_GAMES : voir les parties en cours

- OBSERVE <id> : observer une partie

- OUT_OBSERVER : lorsque vous observez une partie, pour en sortir

- CHALLENGE <user> : défier un joueur

- ACCEPT <user> / REFUSE <user> : lorsqu'on vous a défié, pour gérer le défi

- MESSAGE <user> <message> : message privé

- SAY <message> : discuter globalement

- BIO <texte> : définir ou mettre à jour votre bio

- SHOWBIO <user> : afficher la bio

- FRIEND <user> / UNFRIEND <user> : gérer la liste d’amis

- PRIVATE ON|OFF : activer/désactiver le mode privé

- QUIT : quitter le client
