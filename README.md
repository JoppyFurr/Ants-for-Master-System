Ants for Master System
======================

Ants was a PC game originally developed by Miroslav Nemecek.
It was released as an example game created with their [Gemtree Peter](https://github.com/Panda381/Peter) visual programming tool.


This is my remake of the game for the Sega Master System

## How to Play

Each player has a castle and a fence. The goal is to reach a castle height of 100, or to bring the opponent's castle down to height 0. Moves are made by playing cards. Each card has a resource cost (bricks, weapons, or crystals), and an effect (building, attacking, destroying resources, etc) The resources are generated at the start of each turn by your production (builders, soldiers, magi)

The game includes 30 cards, split into three colours:

Red cards (building):

 * Wall, Defense, and Fence all increase the height of your fence
 * Base, Tower, Fort, and Babylon all increase the height of your castle
 * Reserve will decrease your fence by four, but increase your castle by eight
 * School will give you a builder, increasing the rate you produce bricks
 * Wain will decrease the enemy castle by four, and increase your castle by eight

Green cards (army):

 * Archer, Knight, Rider, Platoon, Attack, and Banshee all attack the enemy. (Their fence takes damage first, and the castle takes damage once the fence has been destroyed)
 * Recruit will give you a solder, increasing the rate you produce weapons
 * Saboteur will destroy four each of the enemy bricks, weapons, and crystals
 * Thief will steal up to five each off the enemy bricks, weapons, and crystals
 * Swat will decrease the enemy castle by ten, ignoring the fence

Blue cards (magic):

 * Conjure cards will generate eight bricks, weapons, or crystals
 * Crush cards will destroy eight enemy bricks, weapons, or crystals
 * Sorcerer will give you a magi, increasing the rate you produce crystals
 * Dragon will attack the enemy
 * Pixies will increase the height of your castle
 * Curse will gain you one of everything, and take from the enemy one of everything. Including production. (The enemy will always retain a minimum of 1 production of each type)

I hope you enjoy playing :3

## Tools & Libraries used
* [SDCC 4.3](https://sdcc.sourceforge.net/) <br /> Small Devices C Compiler
* [devkitSMS](https://github.com/sverx/devkitSMS) <br /> Tools & libraries for Sega Master System development
* [pcmenc](https://github.com/maxim-zhao/pcmenc) <br /> Encodes .wav PCM audio for playback on 8-bit sound chips
* [sms-fxsample](https://github.com/kusfo/sms-fxsample) <br /> C wrapper for PCM playback
* [Sneptile](https://github.com/JoppyFurr/Sneptile) <br /> Convert .png images into VDP patterns
* The clouds in the background have been borrowed from Bridge Zone of SEGA's Sonic The Hedgehog (SMS version)
