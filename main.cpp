#include <iostream>
#include <SDL2/SDL.h>

#undef main
int main()
{
    //start  initialization
 
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window* window = SDL_CreateWindow("1.CPP SDL2 PONG GEME By SahajMansuri ",50,50,800,600,SDL_WINDOW_SHOWN);
    SDL_Event e;
    SDL_Renderer* renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
    bool quit =false;

    
    SDL_Rect playerPaddle;
    playerPaddle.x = 20;
    playerPaddle.y = (600/2)-(100/2);
    playerPaddle.w = 20;
    playerPaddle.h = 100;

    SDL_Rect secondPaddle;
    secondPaddle.x = 800 - 20 - 20;
    secondPaddle.y = (600/2)-(100/2);
    secondPaddle.w = 20;
    secondPaddle.h = 100;

    SDL_Rect ball;
    ball.x = (800/2) - (20/2);
    ball.y = (600/2) - (20/2);
    ball.w = ball.h = 20;

    float bx = ball.x;
    float by = ball.y;

    float speedX = (rand() % 5); // (rend() % 5) - 2  => -2, -1, 0, 1, 2;
    float speedY = (rand() % 5);

    if(speedX >= -0.01f && speedX <= 0.01f) speedX = 1.0f;  //-2, -1, 1.4, 1, 2;
    if(speedY >= -0.01f && speedY <= 0.01f) speedY = -1.0f; //-2, -1, -1.4, 1, 2;

    speedX /= 20.0f; 
    speedY /= 20.0f;

    SDL_Rect score;
    score.x = (800/2) - 20;
    score.y = 0;
    score.w = 40;
    score.h = 20;

    bool AIscore = false;
    bool Playerscore = false;

    while (!quit)
    {
        //Event 
        while (SDL_PollEvent(&e) > 0)
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
            else if(e.type == SDL_KEYDOWN)
            {
                if(e.key.keysym.sym == SDLK_UP)
                    playerPaddle.y -= 30;
                else if(e.key.keysym.sym == SDLK_DOWN)
                    playerPaddle.y += 30;
                if(playerPaddle.y <0)
                    playerPaddle.y = 0;
                else if(playerPaddle.y > 600 -playerPaddle.h)
                    playerPaddle.y = 600 -playerPaddle.h;
            }
            else if(e.type == SDL_MOUSEMOTION)
            {
                SDL_GetMouseState(&playerPaddle.x , &playerPaddle.y);
                playerPaddle.x = 20;
                playerPaddle.y -= playerPaddle.h/2;
                if(playerPaddle.y <0)
                    playerPaddle.y = 0;
                else if(playerPaddle.y > 600 -playerPaddle.h)
                    playerPaddle.y = 600 -playerPaddle.h;
            }            
        }
        
        //update

        bx += speedX; 
        by += speedY;

        //boundaries
        //left
        if(bx <=0.0f)
        {
            bx = 0.0f;
            speedX *= -1.0f;

            score.x -= 100;

            Playerscore = true;
            AIscore = false;

            ball.x = (800/2) - (20/2);
            ball.y = (600/2) - (20/2);
            bx = ball.x;
            by = ball.y;
            speedX = (rand() % 5); // (rend() % 5) - 2  => -2, -1, 0, 1, 2;
            speedY = (rand() % 5);
            if(speedX >= -0.01f && speedX <= 0.01f) speedX = 1.0f;  //-2, -1, 1.4, 1, 2;
            if(speedY >= -0.01f && speedY <= 0.01f) speedY = -1.0f; //-2, -1, -1.4, 1, 2;
            speedX /= 20.0f; 
            speedY /= 20.0f;
        }
        //right
        if(bx >= 800.0f - ball.w)
        {
            bx = 800.0f - ball.w;
            speedX *= -1.0f;

            score.x += 100;

            AIscore = true;
            Playerscore = false;

            ball.x = (800/2) - (20/2);
            ball.y = (600/2) - (20/2);
            bx = ball.x;
            by = ball.y;
            speedX = (rand() % 5); // (rend() % 5) - 2  => -2, -1, 0, 1, 2;
            speedY = (rand() % 5);
            if(speedX >= -0.01f && speedX <= 0.01f) speedX = 1.0f;  //-2, -1, 1.4, 1, 2;
            if(speedY >= -0.01f && speedY <= 0.01f) speedY = -1.0f; //-2, -1, -1.4, 1, 2;
            speedX /= 20.0f; 
            speedY /= 20.0f;
        }
        //top
        if(by <=0.0f)
        {
            by = 0.0f;
            speedY *= -1.0f;
        }
        //bottom
        if(by >= 600.0f - ball.h)
        {
            by = 600.0f - ball.h;
            speedY *= -1.0f;
        }

        //player paddle-ball collision only right side collision
        if(ball.x >= playerPaddle.x && ball.x < playerPaddle.x + playerPaddle.w
         &&ball.y >= playerPaddle.y && ball.y < playerPaddle.y + playerPaddle.h)
        {
            ball.x = playerPaddle.x + playerPaddle.w;
            speedX = abs(speedX);
        }

        // AI Paddle 
        if(secondPaddle.y + (secondPaddle.h/2) > ball.y + (ball.h/2) && rand() % 25 == 0)
        {
            secondPaddle.y--;
        }
        else if(secondPaddle.y + (secondPaddle.h/2) < ball.y + (ball.h/2) && rand() % 25 == 0)
        {
            secondPaddle.y++;
        }
        
        if(secondPaddle.y <0)
            secondPaddle.y = 0;
        else if(secondPaddle.y > 600 -secondPaddle.h)
            secondPaddle.y = 600 -secondPaddle.h;

        //second paddle-ball collision only left side collision
        if(ball.x + ball.w >= secondPaddle.x && ball.x + ball.w < secondPaddle.x + secondPaddle.w
        && ball.y >= secondPaddle.y && ball.y < secondPaddle.y + secondPaddle.h)
        {
            ball.x = secondPaddle.x - ball.w;
            speedX = -abs(speedX);
        }

        ball.x = bx;
        ball.y = by;

        //render / draw Drawing.....
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if(AIscore)
        {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        }
        else if(Playerscore)
        {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); 
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 255, 0, 255, 0);
        }
        SDL_RenderDrawRect(renderer, &score);
      
        
        
        SDL_SetRenderDrawColor(renderer, 255, 143, 123, 255);
        SDL_Rect rect;
        rect.x = 0;
        rect.y = 0;
        rect.w = 400;
        rect.h = 600;
        SDL_RenderDrawRect(renderer, &rect);

        // CREATE A CENTER line 
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);
        SDL_RenderDrawLine(renderer, 800/2, 0, 800/2, 600);

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        rect.x = 400;
        rect.y = 0;
        rect.w = 400;
        rect.h = 600;
        SDL_RenderDrawRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderFillRect(renderer, &playerPaddle);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &secondPaddle);
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &ball);

        SDL_RenderPresent(renderer);

        //drawing paddles
    }
    
    //End
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}