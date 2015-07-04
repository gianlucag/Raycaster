//============================================================================
// Name        : raycaster.cpp
// Author      : Gianluca Ghettini
// Version     : 0.1
// Copyright   : 
// Description : Raycaster in C++. texture mapping, floor, ceiling, skydome
//============================================================================

#include <iostream>
#include <math.h>
#include <cstdio>
#include "PixelToaster.h"

using namespace std;
using namespace PixelToaster;

// risoluzione
#define SCREEN_W 640
#define SCREEN_H 480

#define TEXT_W 256
#define TEXT_H 256

// Field Of View
#define FOV 0.66

// Pi
#define PI 3.14159

// velocita' di movimento e rotazione
#define MOVSPEED 0.1
#define ROTSPEED 0.05

// disegnare pavimento
#define CAST_FLOOR true

// disegnare soffitto
#define CAST_CEILING true

// disegnare skydome
#define CAST_SKYDOME true

// modalita' di movimento (WASD+mousefreelook o solo tastiera WASD)
#define MOUSE_FREELOOK true

// texture mapping on/off 
#define TEXTURE_MAPPING true

// texture
struct Texture
{
	uint32_t width;
	uint32_t height;
	TrueColorPixel *data;
};

// framebuffer
struct Frame
{
	uint32_t width;
	uint32_t height;
	TrueColorPixel *data;
};

// mappa 
struct World
{
	uint32_t width;
	uint32_t height;
	uint8_t *data;
};

// stato della telecamera (posizione, direzione, piano di proiezione)
struct State
{
	double posx;
	double posy;
	double dirx;
	double diry;
	double camx;
	double camy;
};

// informazioni sul raggio
struct RayHit
{
	double distance;
	int mapX;
	int mapY;
	double wallX;
	double rayDirX;
	double rayDirY;
	int side;
	uint32_t blockOffset;
};

// texture
Texture brick, wood, stone, tiles, concrete, ceiling, skydome;

bool LoadWorld(const char filename[], World *world)
{
	FILE* file;
	char string[1024];
	
	file = fopen(filename, "rb");
	if (!file)
	{
		return false;
	}

	// dimensioni della mappa
	fgets(string, 1024, file);
	world->width = atoi(string);
	fgets(string, 1024, file);
	world->height = atoi(string);

	// allocazione mappa
	world->data = new uint8_t[world->width * world->height];

	// caricamento mappa
	for (uint32_t h = 0; h < world->height; h++)
	{
		fgets(string, 1024, file);

		for (uint32_t w = 0; w < world->width; w++)
		{
			if (string[w] == 0x20)
			{
				// blocco vuoto
				world->data[w + h * world->width] = 0;
			}
			else
			{
				// muro (conversione ASCII a intero)
				world->data[w + h * world->width] = (uint8_t)(string[w] - 0x30);
			}
		}
	}

	return true;
}

bool LoadTexture(const char filename[], Texture *tex)
{
	FILE *file;

	file = fopen(filename, "rb");
	if(!file)
	{
		return false;
	}

	// legge l'header TGA
	unsigned char header[18];
	fread(header, 18, 1, file);

	// 2 = formato non RLE compresso
	if(header[2] != 2)
	{
		return false;
	}

	// colordepth deve essere 24 bit (formato RGB_888)
	if(header[16] != 24)
	{
		return false;
	}

	// caricamento dimensioni texture
	tex->width = (header[13] << 8 ) | header[12];
	tex->height = (header[15] << 8 ) | header[14];

	// allocazione memoria
	tex->data = new TrueColorPixel[tex->width * tex->height];
	
	// caricamento immagine
	uint32_t i = 0;
	for (uint32_t y = 0; y < tex->height; y++)
	{
		for (uint32_t x = 0; x < tex->width; x++)
		{
			tex->data[i].b = fgetc(file); // R
			tex->data[i].g = fgetc(file); // G
			tex->data[i].r = fgetc(file); // B

			i++;
		}
	}
	
	fclose(file);

	return true;
}

class Listen : public Listener
{
public:

	State *state;
	World *world;
	double moveSpeed;
	double rotSpeed;

	float mousex;
	float mousey;

	Listen()
	{
		mousex = 0;
		mousey = 0;
	}

protected:

	void onMouseMove(DisplayInterface & display, Mouse mouse)
	{
		if(!MOUSE_FREELOOK)
		{
			return;
		}
		
		if (mouse.x > mousex) // gira a destra
		{

			float deltax = mouse.x - mousex;
			float rotSpeed = deltax / 400;
			mousex = mouse.x;

			double oldDirX = state->dirx;
			state->dirx = state->dirx * cos(-rotSpeed) - state->diry
					* sin(-rotSpeed);
			state->diry = oldDirX * sin(-rotSpeed) + state->diry
					* cos(-rotSpeed);
			double oldcamx = state->camx;
			state->camx = state->camx * cos(-rotSpeed) - state->camy
					* sin(-rotSpeed);
			state->camy = oldcamx * sin(-rotSpeed) + state->camy
					* cos(-rotSpeed);

		}
		else // gira a sinistra
		{
			float deltax = mousex - mouse.x;
			float rotSpeed = deltax / 400;
			mousex = mouse.x;

			double oldDirX = state->dirx;
			state->dirx = state->dirx * cos(rotSpeed) - state->diry
					* sin(rotSpeed);
			state->diry = oldDirX * sin(rotSpeed) + state->diry * cos(rotSpeed);
			double oldcamx = state->camx;
			state->camx = state->camx * cos(rotSpeed) - state->camy
					* sin(rotSpeed);
			state->camy = oldcamx * sin(rotSpeed) + state->camy
					* cos(rotSpeed);
		}

	}

	void onKeyPressed(DisplayInterface & display, Key key)
	{
		if (key == Key::W)
		{
			int currX = (int)state->posx;
			int currY = (int)state->posy;
			int nextX = (int)(state->posx + state->dirx * moveSpeed * 2);
			int nextY = (int)(state->posy + state->diry * moveSpeed * 2);

			if (world->data[nextX + currY * world->width] == 0)
			{
				state->posx += state->dirx * moveSpeed;
			}

			if (world->data[currX + nextY * world->width] == 0)
			{
				state->posy += state->diry * moveSpeed;
			}
		}

		if (key == Key::S)
		{
			int currX = (int)state->posx;
			int currY = (int)state->posy;
			int nextX = (int)(state->posx - state->dirx * moveSpeed * 2);
			int nextY = (int)(state->posy - state->diry * moveSpeed * 2);

			if (world->data[nextX + currY * world->width] == 0)
			{
				state->posx -= state->dirx * moveSpeed;
			}
			if (world->data[currX + nextY * world->width] == 0)
			{
				state->posy -= state->diry * moveSpeed;
			}
		}
		if (key == Key::A) 
		{
			if(MOUSE_FREELOOK) // strafe a sinistra
			{
				int currX = (int)state->posx;
				int currY = (int)state->posy;
				int nextX = (int)(state->posx - state->diry * moveSpeed * 2);
				int nextY = (int)(state->posy + state->dirx * moveSpeed * 2);

				if (world->data[nextX + currY * world->width] == 0)
				{
					state->posx -= state->diry * moveSpeed;
				}

				if (world->data[currX + nextY * world->width] == 0)
				{
					state->posy += state->dirx * moveSpeed;
				}
			}
			else // gira a sinistra
			{
				double oldDirX = state->dirx;
				state->dirx = state->dirx * cos(rotSpeed) - state->diry * sin(rotSpeed);
				state->diry = oldDirX * sin(rotSpeed) + state->diry * cos(rotSpeed);
				double oldcamx = state->camx;    
				state->camx = state->camx * cos(rotSpeed) - state->camy * sin(rotSpeed);
				state->camy = oldcamx * sin(rotSpeed) + state->camy * cos(rotSpeed);
			}
		}
		if (key == Key::D)
		{
			if(MOUSE_FREELOOK) // strafe a destra
			{
				int currX = (int)state->posx;
				int currY = (int)state->posy;
				int nextX = (int)(state->posx + state->diry * moveSpeed * 2);
				int nextY = (int)(state->posy - state->dirx * moveSpeed * 2);

				if (world->data[nextX + currY * world->width] == 0)
				{
					state->posx += state->diry * moveSpeed;
				}

				if (world->data[currX + nextY * world->width] == 0)
				{
					state->posy -= state->dirx * moveSpeed;
				}
			}
			else // gira a destra
			{
				double oldDirX = state->dirx;
				state->dirx = state->dirx * cos(-rotSpeed) - state->diry * sin(-rotSpeed);
				state->diry = oldDirX * sin(-rotSpeed) + state->diry * cos(-rotSpeed);
				double oldcamx = state->camx;
				state->camx = state->camx * cos(-rotSpeed) - state->camy * sin(-rotSpeed);
				state->camy = oldcamx * sin(-rotSpeed) + state->camy * cos(-rotSpeed);
			}
		}
	}
};

void DrawColumn(RayHit what, World world, State state, Frame frame, uint32_t column)
{
	// tipo di blocco rilevato
	uint8_t type = world.data[what.mapX + what.mapY * world.width];

	// seleziona colore in base al tipo di blocco
	uint8_t r, g, b;
	switch (type)
	{
	case 1:
	{
		r = 0;
		g = 255;
		b = 0;
		break;
	}
	case 2:
	{
		r = 155;
		g = 155;
		b = 155;
		break;
	}
	case 3:
	{
		r = 0;
		g = 0;
		b = 255;
		break;
	}
	case 4:
	{
		r = 255;
		g = 0;
		b = 0;
		break;
	}
	}

	// seleziona texture in base al tipo di blocco
	Texture texture;
	switch (type)
	{
	case 1:
	{
		texture = stone;
		break;
	}
	case 2:
	{
		texture = wood;
		break;
	}
	case 3:
	{
		texture = concrete;
		break;
	}
	case 4:
	{
		texture = brick;
		break;
	}
	}



	// calcola altezza colonna
	
	uint32_t colh = abs(int(frame.height / what.distance));

	uint32_t cropup = 0;
	uint32_t cropdown = 0;
	uint32_t index = 0;
	
	
	if (colh > frame.height) // se e' piu' alta dello schermo, taglia
	{
		index = column;
		cropup = (colh - frame.height) / 2;
		cropdown = cropup + 1;
	}
	else
	{
		index = column + ((frame.height - colh) / 2) * frame.width;
		cropup = 0;
		cropdown = 0;
	}

	// disegna colonna
	for (uint32_t c = cropup; c < (colh - cropdown); c++)
	{
		if(TEXTURE_MAPPING) // texture mapping abilitato
		{
			// calcola il pixel da prelevare nella texture
			double d = (double)c / (double)colh;
			int texY = ((int)(d * TEXT_H)) % TEXT_H;
			TrueColorPixel t = texture.data[what.blockOffset + texY * TEXT_W];

			// disegna il pixel della texture
			frame.data[index].r = t.r;
			frame.data[index].g = t.g;
			frame.data[index].b = t.b;
		}
		else // no texture mapping
		{
			// disegna il pixel del colore selezionato 
			frame.data[index].r = r;
			frame.data[index].g = g;
			frame.data[index].b = b;
		}

		index += frame.width;
	}

	
	// posizione X,Y del texel della texture proprio sotto il muro
	double floorXWall, floorYWall;

	// 4 possibili direzioni del muro
	if (what.side == 0 && what.rayDirX > 0)
	{
		floorXWall = what.mapX;
		floorYWall = what.mapY + what.wallX;
	}
	else if (what.side == 0 && what.rayDirX < 0)
	{
		floorXWall = what.mapX + 1.0;
		floorYWall = what.mapY + what.wallX;
	}
	else if (what.side == 1 && what.rayDirY > 0)
	{
		floorXWall = what.mapX + what.wallX;
		floorYWall = what.mapY;
	}
	else
	{
		floorXWall = what.mapX + what.wallX;
		floorYWall = what.mapY + 1.0;
	}

	double distWall, distPlayer, currentDist;
	distWall = what.distance;
	distPlayer = 0.0;

	// disegna pavimento e soffitto
	uint32_t c = (colh + frame.height) / 2;
	
	while (c < frame.height) // per ogni pixel al di sotto della colonna muro
	{
		// calcola la distanza
		currentDist = frame.height / (2.0 * c - frame.height);
		double weight = (currentDist - distPlayer) / (distWall - distPlayer);

		// calcola il punto X,Y nel blocco corrente
		double currentFloorX = weight * floorXWall + (1.0 - weight) * state.posx;
		double currentFloorY = weight * floorYWall + (1.0 - weight) * state.posy;
		
		// calcola il punto X,Y nella texture del pavimento
		int floorTexX, floorTexY;
		floorTexX = int(currentFloorX * 256) % 256;
		floorTexY = int(currentFloorY * 256) % 256;
		
		if(CAST_FLOOR)
		{
			// pixel di pavimento (relativo alla colonna column)
			TrueColorPixel f = tiles.data[floorTexX + floorTexY * 256];
			frame.data[index].r = f.r;
			frame.data[index].g = f.g;
			frame.data[index].b = f.b;
		}

		if(CAST_CEILING)
		{
			// pixel di soffitto (relativo alla colonna column)
			TrueColorPixel g = ceiling.data[floorTexX + floorTexY * 256];
			frame.data[column + (frame.height - c - 1) * frame.width].r = g.r;
			frame.data[column + (frame.height - c - 1) * frame.width].g = g.g;
			frame.data[column + (frame.height - c - 1) * frame.width].b = g.b;
		}
		
		index += frame.width;
		c++;
	}
}

void RenderScene(State state, World world, Frame frame)
{
	uint32_t i = 0;
	
	if(CAST_SKYDOME)
	{
		double angle = atan2(state.dirx, state.diry) + PI; // angolo telecamera (0,2*PI)
		
		uint32_t skyoffset = (int)((double)skydome.width * angle / (double)(2 * PI));
		double stretch = (double)(skydome.width * FOV * 100 / 360) / (double)frame.width;
		
		for(uint32_t i = 0; i < (frame.height / 2); i++)
		{
			for(uint32_t k = 0; k < frame.width; k++)
			{
				uint32_t si = (int)((double)i * ((double)skydome.height / (double)frame.height * 2));
				uint32_t sk = (int)((double)k * (double)stretch);
				
				TrueColorPixel s = skydome.data[(sk + skyoffset) % skydome.width + si * skydome.width];
				
				frame.data[k + i * frame.width].r = s.r;
				frame.data[k + i * frame.width].g = s.g;
				frame.data[k + i * frame.width].b = s.b;
			}
		}
	}
	else
	{
		// flat sky	
		for(i = 0; i < (frame.width * frame.height / 2); i++)
		{
			frame.data[i].r = 135;
			frame.data[i].g = 206;
			frame.data[i].b = 250;
		}
		
		// floor
		for(i = (frame.width * frame.height / 2); i < (frame.width * frame.height); i++)
		{
			frame.data[i].r = 102;
			frame.data[i].g = 51;
			frame.data[i].b = 0;
		}
	}

	for (uint32_t column = 0; column < frame.width; column++) // per ogni colonna
	{
		// calcola la posizione e la direzione del raggio 
		double cameraX = 2 * column / double(frame.width) - 1;
		double rayPosX = state.posx;
		double rayPosY = state.posy;
		double rayDirX = state.dirx + state.camx * cameraX;
		double rayDirY = state.diry + state.camy * cameraX;

		// il blocco attuale dove siamo  
		int mapX = int(rayPosX);
		int mapY = int(rayPosY);

		// lunghezza del raggio dalla posizione attuale al blocco successivo
		double sideDistX;
		double sideDistY;

		// lunghezza del raggio da un blocco ad un altro
		double deltaDistX = sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX));
		double deltaDistY = sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY));
		double perpWallDist;

		// direzione nella quale andare (+1 o -1), sia per X che per Y
		int stepX;
		int stepY;
		int side; // faccia del cubo incontrata (faccia Nord-Sud o faccia Ovest-Est)

		if (rayDirX < 0)
		{
			stepX = -1;
			sideDistX = (rayPosX - mapX) * deltaDistX;
		}
		else
		{
			stepX = 1;
			sideDistX = (mapX + 1.0 - rayPosX) * deltaDistX;
		}
		
		if (rayDirY < 0)
		{
			stepY = -1;
			sideDistY = (rayPosY - mapY) * deltaDistY;
		}
		else
		{
			stepY = 1;
			sideDistY = (mapY + 1.0 - rayPosY) * deltaDistY;
		}

		// algoritmo DDA (raycast)
		while ((world.data[mapX + mapY * world.width] == 0)) // finche' non incontriamo un muro...
		{
			// andiamo al prossimo blocco nella mappa
			if (sideDistX < sideDistY)
			{
				sideDistX += deltaDistX;
				mapX += stepX;
				side = 0;
			}
			else
			{
				sideDistY += deltaDistY;
				mapY += stepY;
				side = 1;
			}
		}

		// calcolo lunghezza del raggio
		if(side == 0)
		{
			perpWallDist = fabs((mapX - rayPosX + (1 - stepX) / 2) / rayDirX);
		}
		else
		{
			perpWallDist = fabs((mapY - rayPosY + (1 - stepY) / 2) / rayDirY);
		}

		// texture

		// calcola wallX ovvero l'offset x del blocco colpito dal raggio
		double wallX;
		if (side == 1)
		{
			wallX = rayPosX + ((mapY - rayPosY + (1 - stepY) / 2) / rayDirY) * rayDirX;
		}
		else
		{
			wallX = rayPosY + ((mapX - rayPosX + (1 - stepX) / 2) / rayDirX) * rayDirY;
		}
		wallX -= floor((wallX));

		// riga della texture (in coordinate texture) 
		int texX = int(wallX * double(TEXT_W));
		
		// inverti in base alla posizione del raggio
		if (side == 0 && rayDirX > 0)
		{
			texX = TEXT_W - texX - 1;
		}
		if (side == 1 && rayDirY < 0)
		{
			texX = TEXT_W - texX - 1;
		}

		// carica RayHit con le informazioni per disegnare la colonna
		RayHit what;
		what.distance = perpWallDist;
		what.blockOffset = texX;
		what.mapX = mapX;
		what.mapY = mapY;
		what.side = side;
		what.rayDirX = rayDirX;
		what.rayDirY = rayDirY;
		what.wallX = wallX;

		// disegna la colonna
		DrawColumn(what, world, state, frame, column);
	}
}

void DrawScene(Display &display, Frame frame)
{
	display.update(frame.data);
}

int main()
{
	// init schermo
	Display display("Raycaster", SCREEN_W, SCREEN_H, Output::Windowed);
	display.open();
	Listen l;
	display.listener(&l);

	
	// init framebuffer
	Frame frame;
	frame.data = new TrueColorPixel[SCREEN_W * SCREEN_H];
	frame.width = SCREEN_W;
	frame.height = SCREEN_H;

	
	// init stato
	State state;
	state.posx = 3;
	state.posy = 3;
	state.dirx = -1;
	state.diry = 0;
	state.camx = 0;
	state.camy = FOV;

	
	// caricamento delle texture
	if (!LoadTexture("brick.tga", &brick))
	{
		printf("\nError loading texture file!");
		exit(0);
	}
	if (!LoadTexture("wood.tga", &wood))
	{
		printf("\nError loading texture file!");
		exit(0);
	}
	if (!LoadTexture("stone.tga", &stone))
	{
		printf("\nError loading texture file!");
		exit(0);
	}
	if (!LoadTexture("floor.tga", &tiles))
	{
		printf("\nError loading texture file!");
		exit(0);
	}
	if (!LoadTexture("concrete.tga", &concrete))
	{
		printf("\nError loading texture file!");
		exit(0);
	}
	if (!LoadTexture("ceil.tga", &ceiling))
	{
		printf("\nError loading texture file!");
		exit(0);
	}
	if (!LoadTexture("skydome.tga", &skydome))
	{
		printf("\nError loading texture file!");
		exit(0);
	}

	// caricamento mappa
	World world;
	if (!LoadWorld("world.txt", &world))
	{
		printf("\nError loading world file!");
		exit(0);
	}

	// init del listener
	l.state = &state;
	l.world = &world;
	l.moveSpeed = MOVSPEED;
	l.rotSpeed = ROTSPEED;
	
	while(1) // loop principale (rendering e disegno della scena)
	{
		RenderScene(state, world, frame);
		DrawScene(display, frame);
	}
}
