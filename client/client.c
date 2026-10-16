#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

SDL_Renderer *CreateRenderer(SDL_Window *win) {
	SDL_Renderer *renderer = SDL_CreateRenderer(win, NULL);
	if (!renderer)
		return 0;

	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
}

int start_client() {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 1;
	}

	SDL_Window *window = SDL_CreateWindow(
		"My Game",
        800, 600,
        SDL_WINDOW_RESIZABLE
	);

	if (!window) {
		fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	SDL_Renderer *renderer = CreateRenderer(window);

	if (!renderer) {
		fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}


	SDL_Event event;
	int running = 1;

	while (running) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT)
				running = 0;
		}
		SDL_Delay(10);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

