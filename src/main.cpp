/*
 * Matthew Todd Geiger <mgeiger@newtonlabs.com>
 *
 * main.cpp
 */

// Standard Library
#include <iostream>
#include <ctime>
#include <mutex>
#include <algorithm>
#include <errno.h>
#include <cstring>
#include <ctime>

// SDL Includes
#include <SDL2/SDL.h>

// X Includes
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>

#ifdef DEBUG
#define DPRINT(str, ...) printf("DEBUG --> " str "\n", ##__VA_ARGS__)
#else
#define DPRINT(str, ...)
#endif

#define EPRINT(str, ...) fprintf(stderr, "ERROR --> " str "\nSTERROR: %s\n", ##__VA_ARGS__, SDL_GetError())

#define FATAL(str, ...) { fprintf(stderr, "FATAL --> " str "\nSTRERROR: %s\n", ##__VA_ARGS__, SDL_GetError()); exit(EXIT_FAILURE); } 

#if defined(UNUSED_PARAMETER)
#error UNUSED_PARAMETER has already been defined!
#else
#define UNUSED_PARAMETER(x) (void)(x)
#endif

#ifndef BASE_DIR
#define BASE_DIR "./"
#endif

#define FONT_DIR "fonts"
#define FONTSIZE 13
#define CIRCLESIZE 10.0f
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define MOVEMENTSPEED 400.0f
#define VELOCITY 600.0
#define JUMPVELOCITY 1000.0
#define ACCELERATION 1200.0

class Clock {
	private:
		std::clock_t lastRecordedTime;
	
	public:
		Clock();
		double GetDeltaTime();	
};

double Clock::GetDeltaTime() {
	std::clock_t curRecordedTime = std::clock();
	double duration = static_cast<double>(curRecordedTime - lastRecordedTime) / static_cast<double>(CLOCKS_PER_SEC);
	lastRecordedTime = curRecordedTime;
	return duration;
}

Clock::Clock() {
	lastRecordedTime = std::clock();
}

class Character {
	private:
		//                    x    y    w   h
		SDL_FRect dimenesions;
		Uint8 cr = 0, cg = 0, cb = 0;

		void CalculateVertical(double &deltaTime, bool spaceKey);
		void CalculateHorizontal(double &deltaTime);

		bool inAir = false;
		bool isJumping = false;

		double velocity = 600.0;
		double acceleration = 1200.0;
		double jumpvelocity = 1200.0;
		double horizontalVelocity = 0;

	public:
		bool wantsToMoveRight = false;
		bool wantsToMoveLeft = false;
		
		Character(float x = 0.0f, float y = 0.0f, float w = 25.0f, float h = 25.0f, Uint8 r = 0, Uint8 g = 0, Uint8 b = 0);
		SDL_FRect* GetDimensions();
		void MovX(float);
		void MovY(float);
		void SetXY(float x, float y);
		void SetWH(float w, float h);
		Uint8 GetR();
		Uint8 GetG();
		Uint8 GetB();
		void Jump();
		void Update(double deltaTime, bool spaceKey = false);
};

Character::Character(float x, float y, float w, float h, Uint8 r, Uint8 g, Uint8 b) {
	SetXY(x, y);
	SetWH(w, h);

	cr = r; cg = g; cb = b;
}

void Character::Jump() {
	if(!inAir && dimenesions.y >= SCREEN_HEIGHT-dimenesions.h) {
		isJumping = true;
		inAir = true;
	}
}

void Character::CalculateVertical(double &deltaTime, bool spaceKey) {
	if(dimenesions.y >= SCREEN_HEIGHT-dimenesions.h) {
		inAir = false;
		if(spaceKey) Jump();
	} else {
		inAir = true;
	}

	double accel = acceleration;

	// Increases downward acceleration when going up
	if(velocity < 0.0) {
		accel *= 1.5;
	}

	if(inAir) {
		velocity += accel*deltaTime;
	} else if(velocity > 0.0) {
		velocity = 0.0-(velocity/2);
		if(velocity > -60)
			velocity = 0;
	}

	if(isJumping) {
		velocity = 0.0-jumpvelocity;
		isJumping = false;
	}
	
	MovY(velocity*deltaTime);
}

void Character::CalculateHorizontal(double &deltaTime) {
	bool wantsToMoveHoz = (wantsToMoveLeft || wantsToMoveRight) ? true : false;

	double hozAccelRight = 0.0-acceleration;
	double hozAccelLeft = acceleration;

	if(inAir) {
		hozAccelLeft /= 1.75;
		hozAccelRight /= 1.75;
	}

	// New horizontal movement includes velocity/acceleration
	if(wantsToMoveHoz && (wantsToMoveRight == false || wantsToMoveLeft == false)) {
		if(wantsToMoveRight) {
			// Kick start acceleration
			if(horizontalVelocity < 3.0 && horizontalVelocity > -3.0) {
				hozAccelLeft *= 200;
			} else if(horizontalVelocity < 5.0 && horizontalVelocity > -5.0) {
				hozAccelLeft *= 100;
			}

			// Boost stopping accel
			if(horizontalVelocity < 0.0) {
				hozAccelLeft *= 1.75;
			}

			horizontalVelocity += hozAccelLeft*deltaTime;
		}

		if(wantsToMoveLeft) {
			// Kick start acceleration
			if(horizontalVelocity < 3.0 && horizontalVelocity > -3.0) {
				hozAccelRight *= 200;
			} else if(horizontalVelocity < 5.0 && horizontalVelocity > -5.0) {
				hozAccelRight *= 100;
			}

			// Boost stopping accel
			if(horizontalVelocity > 0.0) {
				hozAccelRight *= 1.75;
			}

			horizontalVelocity += hozAccelRight*deltaTime;
		}

	} else {
		if(horizontalVelocity > 0.0) {
			horizontalVelocity += hozAccelRight*deltaTime;	
		} else {
			horizontalVelocity += hozAccelLeft*deltaTime;
		}
	}

	if(horizontalVelocity > -0.5 && horizontalVelocity < 0.5) {
		horizontalVelocity = 0;
	}

	MovX(horizontalVelocity*deltaTime);

}

void Character::Update(double deltaTime, bool spaceKey) {
	CalculateVertical(deltaTime, spaceKey);
	CalculateHorizontal(deltaTime);
}

Uint8 Character::GetR() {
	return cr;
}

Uint8 Character::GetG() {
	return cg;
}

Uint8 Character::GetB() {
	return cb;
}

void Character::MovX(float x) {
	dimenesions.x = std::clamp<float>(dimenesions.x + x, 0.0f, SCREEN_WIDTH-dimenesions.w);
	if(dimenesions.x == 0.0f || dimenesions.x == SCREEN_WIDTH-dimenesions.w)
		horizontalVelocity = 0.0;
}

void Character::MovY(float y) {
	dimenesions.y = std::clamp<float>(dimenesions.y + y, 0.0f, SCREEN_HEIGHT-dimenesions.h);
}

void Character::SetWH(float w, float h) {
	dimenesions.w = w;
	dimenesions.h = h;
}

void Character::SetXY(float x, float y) {
	dimenesions.x = x;
	dimenesions.y = y;
}

SDL_FRect* Character::GetDimensions() {
	return &dimenesions;
}


enum Keys {KEY_A = 0, KEY_D, KEY_S, KEY_W, KEY_SPACE, KEY_ESC};

class App {
	private:
		bool keys[6];
		bool running;
		double deltaTime;
		SDL_Renderer* renderer;
		SDL_Window* window;

		Character *player;
		Clock *timer;

		// In Loop
		void HandleKeydown(SDL_Keycode);
		void HandleKeyup(SDL_Keycode);
		void HandleKeys();

		// In render loop
		void PrepareScene();
		void PresentScene();

	public:
		App();

		int OnExecute();
		bool OnInit();
		void OnEvent(SDL_Event* event);
		void OnLoop();
		void OnRender();
		void OnCleanup();

};

// PRIVATES

void App::HandleKeys() {
	if(keys[KEY_A]) {
		//player->MovX(0.0-(MOVEMENTSPEED*deltaTime)); // Old Movement style
		player->wantsToMoveLeft = true;
	} else {
		player->wantsToMoveLeft = false;
	}

	if(keys[KEY_D]) {
		//player->MovX(MOVEMENTSPEED*deltaTime); // Old movement style
		player->wantsToMoveRight = true;
	} else {
		player->wantsToMoveRight = false;
	}

	if(keys[KEY_SPACE])
		player->Jump();
	if(keys[KEY_ESC])
		running = false;
}

void App::HandleKeydown(SDL_Keycode keycode) {
	switch(keycode) {
		case SDLK_a: 		keys[KEY_A] 	= true; break;
		case SDLK_d: 		keys[KEY_D] 	= true; break;
		case SDLK_SPACE: 	keys[KEY_SPACE] = true; break;
		case SDLK_ESCAPE: 	keys[KEY_ESC] 	= true; break;
	}
}

void App::HandleKeyup(SDL_Keycode keycode) {
	switch(keycode) {
		case SDLK_a: 		keys[KEY_A] 	= false; break;
		case SDLK_d: 		keys[KEY_D] 	= false; break;
		case SDLK_SPACE: 	keys[KEY_SPACE] = false; break;
		case SDLK_ESCAPE: 	keys[KEY_ESC] 	= false; break;
	}
}

void App::PrepareScene() {
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDrawRectF(renderer, player->GetDimensions());
	SDL_SetRenderDrawColor(renderer, player->GetR(), player->GetG(), player->GetB(), 255);
}

void App::PresentScene() {
	SDL_RenderPresent(renderer);
}

// PUBLICS
bool App::OnInit() {
	// Init SDL
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		EPRINT("Failed to init SDL!");
		return false;
	}

	// Create window
	if((window = SDL_CreateWindow("SDL Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
					SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_VULKAN)) == nullptr) {
		EPRINT("Failed to create window!");
		return false;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	// Create Renderer	
	if((renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED)) == nullptr) {
		EPRINT("Failed to create renderer");
		return false;
	}

	player = new Character();
	SDL_FRect *playerInfo = player->GetDimensions();	
	player->SetXY(static_cast<float>(SCREEN_WIDTH)/2 - playerInfo->w/2, static_cast<float>(SCREEN_HEIGHT)/2 - playerInfo->h/2);

	timer = new Clock;

	DPRINT("App::OnInit() Completed");

	return true;
}

void App::OnEvent(SDL_Event* event) {
	switch(event->type) {
		case SDL_QUIT:
			running = false;
			break;
		case SDL_KEYDOWN:
			HandleKeydown(event->key.keysym.sym);
			break;
		case SDL_KEYUP:
			HandleKeyup(event->key.keysym.sym);
			break;
		default:
			break;
	}
}

void App::OnLoop() {
	HandleKeys();
	player->Update(deltaTime, keys[KEY_SPACE]);
}

void App::OnRender() {
	PrepareScene();
	PresentScene();
}

void App::OnCleanup() {
	delete player;
	delete timer;
	DPRINT("App::OnCleanup() Completed");
}

App::App() {
	running = true;
}

int App::OnExecute() {
	DPRINT("App::OnExecute() Starting");
	if(OnInit() == false)
		return -1;

	SDL_Event event;

	while(running) {
		deltaTime = timer->GetDeltaTime();

		// Handle SDL Events
		while(SDL_PollEvent(&event)) {
			OnEvent(&event);
		}

		// Game Logic
		OnLoop();

		// Render...
		OnRender();
	}

	// Cleanup your nasty ass
	OnCleanup();

	return 0;
}

int main() {
	App *application = new App;

	if(application->OnExecute() < 0)
		FATAL("Application FATAL Error!");

	delete application;

	return EXIT_SUCCESS;
}
