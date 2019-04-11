#include<iostream>
#include<SDL.h>
#include<iomanip>
#include<bitset>
#include<math.h>
#include<algorithm>

bool init();
Uint32 getPixel(SDL_Surface* texture, const int& x, const int& y);
int map(const int& value, const int& min1, const int& min2, const int& max1, const int& max2);
void drawVeritcalLine(SDL_Renderer* renderer, const int& x, const int& y1, const int& y2, const Uint32& color);
void clearYBuffer();
void renderTerrain(const int& x,const int& y,const float& phi,const float& height,const float& horizon,const float& scaleHeight,const float& distance);
int euclideanMod(const int& a, const int& base);

const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 400;

int yBuffer[SCREEN_WIDTH];

SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;
SDL_Surface* gTerrainTexture = nullptr;
SDL_Surface* gTerrainHeightMap = nullptr;
SDL_Event gEvent;

bool gShouldQuit;

float gX = 0;
float gY = 0;
float gRot = 0;


int main(int argc, char* argv[])
{
	//initialize and run
	if (init())
	{
		//load the terrain maps
		gTerrainTexture = SDL_LoadBMP("C1Wlarge.bmp");
		if (gTerrainTexture == nullptr)
		{
			std::cerr << "ERROR: Couldnt read in texture file" << std::endl;
			exit(-1);
		}
		gTerrainHeightMap = SDL_LoadBMP("D1large.bmp");
		if (gTerrainHeightMap == nullptr)
		{
			std::cerr << "ERROR: Couldnt read in height map file" << std::endl;
			exit(-1);
		}

		//main rendering loop
		gShouldQuit = false;
		while (1)
		{
			//Poll events
			while (SDL_PollEvent(&gEvent))
			{
				switch (gEvent.type)
				{
				case SDL_QUIT:
					gShouldQuit = true;
					break;
				case SDL_KEYDOWN:
					switch (gEvent.key.keysym.sym)
					{
					case SDLK_LEFT:
						gX -= 5.0;
						break;
					case SDLK_RIGHT:
						gX += 5.0;
						break;
					case SDLK_UP:
						gY -= 5.0;
						break;
					case SDLK_DOWN:
						gY += 5.0;
						break;
					case SDLK_z:
						gRot += 0.05;
						break;
					case SDLK_x:
						gRot -= 0.05;
						break;

					default:
						break;
					}

				default:
					break;
				}
			}

			if (gShouldQuit)
			{
				break;
			}

			SDL_SetRenderDrawColor(gRenderer, 32, 100, 200,255);
			SDL_RenderClear(gRenderer);

			//render terrain

			renderTerrain(gX, gY, gRot, 130, 160, 240, 800);
			SDL_RenderPresent(gRenderer);
		}

		SDL_DestroyRenderer(gRenderer);
		SDL_DestroyWindow(gWindow);
		SDL_Quit();
	}
	else
	{
		return -1;
	}
	return 0;
}

bool init()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		return false;
	}
	gWindow = SDL_CreateWindow("Terrain Renderer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	if (gWindow == nullptr)
	{
		return false;
	}
	gRenderer = SDL_CreateRenderer(gWindow, -1, 0);
	if (gRenderer == nullptr)
	{
		return false;
	}
	return true;
}

Uint32 getPixel(SDL_Surface* surface, const int& x, const int& y)
{
	int bpp = surface->format->BytesPerPixel;
	//int newX = std::max(0, std::min(x, surface->w));
	//int newY = std::max(0, std::min(y, surface->h));
	int newX = euclideanMod(x, surface->w);
	int newY = euclideanMod(y, surface->h);

	Uint8* p = (Uint8*)surface->pixels + newY * surface->pitch + newX * bpp;

	//std::cout << std::hex << *(Uint32*)p << std::endl;

	switch (bpp) {
	case 1:
		return *p;
		break;

	case 2:
		return *(Uint16*)p;
		break;

	case 3:
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			return p[0] << 16 | p[1] << 8 | p[2];
		else
			return p[0] | p[1] << 8 | p[2] << 16;
		break;

	case 4:
		return *(Uint32*)p;
		break;

	default:
		return 0;
	}
}

int map(const int& value, const int& min1, const int& min2, const int& max1, const int& max2)
{
	float newValue = value - min1;
	newValue = newValue / (min2 - min1);
	newValue = newValue * (max2 - max1);
	newValue += max1;
	return (int)(newValue);
}

void clearYBuffer()
{
	for (int i = 0; i < SCREEN_WIDTH; i++)
	{
		yBuffer[i] = SCREEN_HEIGHT;
	}
}

void renderTerrain(const int& x, const int& y, const float& phi, const float& height, const float& horizon, const float& scaleHeight, const float& distance)
{
	float sinphi = sin(phi);
	float cosphi = cos(phi);

	float dz = 1.0f;
	float z = 1.0f;

	clearYBuffer();

	while (z < distance)
	{
		
		float pLeftX  = (-cosphi*z - sinphi*z) + x;
		float pLeftY  = ( sinphi*z - cosphi*z) + y;
		float pRightX = ( cosphi*z - sinphi*z) + x;
		float pRightY = (-sinphi*z - cosphi*z) + y;

		float dx = (pRightX - pLeftX) / SCREEN_WIDTH;
		float dy = (pRightY - pLeftY) / SCREEN_WIDTH;

		for (int i = 0; i < SCREEN_WIDTH; i++)
		{
			float heightOnScreen = (((height - (0x000000ff & getPixel(gTerrainHeightMap, pLeftX, pLeftY))) / z * scaleHeight) + horizon);
			
			drawVeritcalLine(gRenderer, i, heightOnScreen, yBuffer[i], getPixel(gTerrainTexture, pLeftX, pLeftY));
			if (heightOnScreen < yBuffer[i])
			{
				yBuffer[i] = heightOnScreen;
			}
			pLeftX += dx;
			pLeftY += dy;
		}
		z += dz;
		dz += 0.005f;
	}
}

void drawVeritcalLine(SDL_Renderer* renderer, const int& x, const int& y1, const int& y2, const Uint32& color)
{
	if (y1 <= y2)
	{
		SDL_SetRenderDrawColor(gRenderer, (0x00ff0000 & color) >> 16, (0x0000ff00 & color) >> 8, (0x000000ff & color), 255);
		SDL_RenderDrawLine(gRenderer, x, y1, x, y2);
	}
}

int euclideanMod(const int& a, const int& base)
{
	return ((a% base) + base) % base;
}
