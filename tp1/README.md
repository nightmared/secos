# TP1 - La segmentation

Le but du TP est de bien comprendre la segmentation.

Vous pouvez directement modifier "tp.c".

Allez lire le fichier "kernel/include/segmem.h". Vous y trouverez plein d'informations, de structures et de macros utiles pour la résolution du TP.

:warning: **QEMU ne supporte pas "complètement" la segmentation.**

Il est nécessaire d'utiliser KVM à la place. Vous devez donc modifier au préalable le fichier utils/config.mk à la ligne:

```bash
 QEMU := $(shell which qemu-system-x86_64)
```

par

```bash
 QEMU := $(shell which kvm)
```


## Questions

### Question 1

**Grub a démarré notre noyau en mode protégé. Il a donc configuré une GDT avant d'exécuter notre point d'entrée. Affichez le contenu de cette GDT. Que constatez-vous ?**

**Servez-vous des outils présents dans notre OS (`get_gdtr(), seg_desc_t et gdt_reg_t`)**

---

### Question 2

**Configurez votre propre GDT contenant des descripteurs ring 0:**
 - **Code, 32 bits RX, flat, indice 1**
 - **Données, 32 bits RW, flat, indice 2**

**Vous pouvez placer ces descripteurs où vous le souhaitez dans la GDT. Attention de bien respecter les restrictions matérielles :**
 - **La GDT doit avoir une adresse de base alignée sur 8 octets**
 - **Le premier descripteur (indice 0) doit être NULL**

**Chargez cette GDT, puis initialisez les registres de segments (cs/ss/ds/...) avec les bons sélecteurs afin qu'ils pointent vers vos nouveaux descripteurs.**

---

### Question 3

**Essayez d'exécuter le code suivant :**

```c
  #include <string.h>

  char  src[64];
  char *dst = 0;

  memset(src, 0xff, 64);
```

**Configurez un nouveau descripteur de données à l'index de votre choix :**
 - **data, ring 0**
 - **32 bits RW**
 - **base 0x600000**
 - **limite 32 octets**

**Chargez le registre de segment "es" de manière à adresser votre nouveau descripteur de données. Puis exécutez le code suivant :**

```c
  _memcpy8(dst, src, 32);
```

**Que se passe-t-il ? Pourquoi n'y a-t-il pas de faute mémoire alors que le pointeur `dst` est NULL ?**
Alors c'est parce que _memcpy utilise une instruction spéciale du nom de rep movsb, qui est en réalité le combo de deux instructions: rep pour répéter un certain nombre de fois l'opération et movsb qui déplace un octet d'une source vers une destination. Et là où c'est magique, c'est quand on regarde le manuel Intel: "Move  byte from address DS:(E)SI to ES:(E)DI.". Ce qui signifie que tu copie les données de DS>-addr+ESI à DS->addr+ESI+ECX vers ES->addr+EDI jusqu'à ES->addr+EDI+ECX (c'est la magie de rep, tu repète l'opération ECX fois). Vu que tu as changée le descripteur de segment pour ES pour metter son adresse de base à celle du buffer de destination, tu ne copies pas sur le même espace d'adressage que l'adressage source. À la place, tu copies vers un espace d'adressage qui commence à l'adresse de ton buffer. Donc ES:0x0 == DS:adresse_destination == adresse_destination physiquement puisque DS a une adresse de base de 0. Du coup c'est absolument pas un pointeur null une fois traduit par le processeur, et tout est bien qui finit bien.

**Effectuez à présent une copie de 64 octets. Que se passe-t-il ?**
Là on dépasse la limite de ton segment, et on tombe dans le côté sécurité de la segmentation. Ton segment fait 32 octets, tu cherches à écrire au delà, et tu te prends une exception à cause de ça. C'est d'ailleurs ce que dit (encore !) la doc Intel:
The processor causes a general-protection exception (or, if the segment is SS, a stack-fault exception) any time an attempt is made to access the following addresses in a segment:
- A byte at an offset greater than the effective limit
- A word at an offset greater than the (effective-limit – 1)
- A doubleword at an offset greater than the (effective-limit – 3)
Ton movsb va bien marcher jusqu'à ce que tu dépasses la  taille du segment + 1, et tu vas te prendre une GP
