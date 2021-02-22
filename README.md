# Datakommunikasjon

Klient-serverapplikasjon i C som sammenligner bilder som er lagret på to endesystemer. 

Applikasjonene skal sende bildedata som pakker fra klient til server og server sjekker bildedataene opp i mot en liste med bildenavn som befinner seg på serveren. 

Bildedata skal sendes fra klient til server ved hjelp av UDP- protokoll med tilleggsfunksjoener som flytkontroll og loss recovery. Sliding window- algortimen blir brukt for å få til dette. 
