#include "SDL.h"
#include <iostream>
#include <vector>
#include <unordered_set>
#include <unordered_map>

struct Cell
{
    int val = 0;
    std::unordered_set<int> not;
    std::unordered_set<Cell *> mates;
};

class Board
{
  public:
    std::vector<Cell> cells = std::vector<Cell>(100);

    Board()
    {
        for (int r = 1; r <= 9; r++)
        {
            for (int c = 1; c <= 9; c++)
            {
                int i = r * 10 + c;
                cells[i].mates = GetMatesFor(i);
            }
        }
    }

    bool RemoveObviousImpossibilities()
    {
        bool changed = false;

        for (int n = 1; n <= 9; n++)
        {
            for (int p = 1; p <= 9; p++)
            {
                int i = n * 10 + p;
                Cell &cell = cells[i];

                if (!cell.val)
                    continue;

                for (auto mate : cell.mates)
                {
                    mate->not.insert(cell.val);
                    changed = true;
                }
            }
        }

        return changed;
    }

    void PromoteAnySingles()
    {
        for (int n = 1; n <= 9; n++)
        {
            for (int p = 1; p <= 9; p++)
            {
                int i = n * 10 + p;
                Cell &cell = cells[i];

                if (cell.val)
                    continue;

                if (cell.not.size() == 8)
                {
                    for (int v = 1; v <= 9; v++)
                    {
                        if (cell.not.count(v) == 0)
                        {
                            cell.val = v;
                            break;
                        }
                    }
                }
            }
        }
    }

  private:
    std::unordered_set<Cell *> GetMatesFor(int i)
    {
        std::unordered_set<Cell *> result;

        int non = i / 10;
        int pos = i % 10;

        /*
        11 12 13  21 22 23  31 32 33
        14 15 16  24 25 26  34 35 36
        17 18 19  27 28 29  37 38 39

        41 42 43  51 52 53  61 62 63
        44 45 46  54 55 56  64 65 66
        47 48 49  57 58 59  67 68 69

        71 72 73  81 82 83  91 92 93
        74 75 76  84 85 86  94 95 96
        77 78 79  87 88 89  97 98 99
        */

        // nonet mates
        for (int p = 1; p <= 9; p++)
            result.insert(&cells[non * 10 + p]);

        // row mates
        {
            int nonMin = non - (non - 1) % 3;
            int nonMax = 2 - (non - 1) % 3 + non;
            int posMin = pos - (pos - 1) % 3;
            int posMax = 2 - (pos - 1) % 3 + pos;

            for (int n = nonMin; n <= nonMax; n++)
                for (int p = posMin; p <= posMax; p++)
                    result.insert(&cells[n * 10 + p]);
        }

        // col mates
        {
            int nonMin = (non - 1) % 3 + 1;
            int nonMax = nonMin + 6;
            int posMin = (pos - 1) % 3 + 1;
            int posMax = posMin + 6;
            for (int n = nonMin; n <= nonMax; n += 3)
                for (int p = posMin; p <= posMax; p += 3)
                    result.insert(&cells[n * 10 + p]);
        }

        // remove self (made earlier logic less branch heavy)
        result.erase(&cells[non * 10 + pos]);

        return result;
    }
};

struct SDL
{
    const int width = 480;
    const int height = 480;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    std::vector<SDL_Rect> src = std::vector<SDL_Rect>(10);
    std::vector<SDL_Rect> dst = std::vector<SDL_Rect>(1000); // 32kb memory

    int Setup()
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            std::cerr << "SDL_Init: " << SDL_GetError() << std::endl;
            return 1;
        }

        window = SDL_CreateWindow("Sudoku Solver", 100, 100, width, height, SDL_WINDOW_SHOWN);
        if (window == nullptr)
        {
            std::cerr << "SDL_CreateWindow: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return 2;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (renderer == nullptr)
        {
            SDL_DestroyWindow(window);
            std::cerr << "SDL_CreateRenderer: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return 3;
        }

        SDL_Surface *font = SDL_LoadBMP("font.bmp");
        if (font == nullptr)
        {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            std::cerr << "SDL_LoadBMP: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return 4;
        }

        texture = SDL_CreateTextureFromSurface(renderer, font);
        SDL_FreeSurface(font);
        if (texture == nullptr)
        {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            std::cerr << "SDL_CreateTextureFromSurface: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return 5;
        }

        // set up rectangles for src image
        for (int i = 0; i < 10; i++)
        {
            src[i].x = i % 5 * (512 / 5.0f);
            src[i].y = i / 5 * 128;
            src[i].w = 102;
            src[i].h = 128;
        }

        // set up rectangles for dst positions
        for (int seg = 1; seg <= 9; seg++)
        {
            int w = (int)(width / 3.0f);
            int h = (int)(height / 3.0f);
            dst[seg].x = (seg - 1) % 3 * w;
            dst[seg].y = (seg - 1) / 3 * h;
            dst[seg].w = w;
            dst[seg].h = h;

            for (int pos = 1; pos <= 9; pos++)
            {
                int i = seg * 10 + pos;
                int w = (int)(width / 9.0f);
                int h = (int)(height / 9.0f);
                dst[i].x = dst[seg].x + (pos - 1) % 3 * w;
                dst[i].y = dst[seg].y + (pos - 1) / 3 * h;
                dst[i].w = w;
                dst[i].h = h;

                for (int note = 1; note <= 9; note++)
                {
                    int i = seg * 100 + pos * 10 + note;
                    int w = (int)(width / 27.0f);
                    int h = (int)(height / 27.0f);
                    dst[i].x = dst[seg * 10 + pos].x + (note - 1) % 3 * w;
                    dst[i].y = dst[seg * 10 + pos].y + (note - 1) / 3 * h;
                    dst[i].w = w;
                    dst[i].h = h;
                }
            }
        }

        return 0;
    }

    ~SDL()
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void Render(Board &b)
    {
        SDL_RenderClear(renderer);

        for (int r = 1; r <= 9; r++)
        {
            for (int c = 1; c <= 9; c++)
            {
                int i = r * 10 + c;
                Cell &cell = b.cells[i];

                if (cell.val)
                {
                    SDL_RenderCopy(renderer, texture, &src[cell.val], &dst[i]);
                }
                else
                {
                    for (int v = 1; v <= 9; v++)
                    {
                        if (cell.not.find(v) != cell.not.end())
                            continue;
                        SDL_RenderCopy(renderer, texture, &src[v], &dst[i * 10 + v]);
                    }
                }
            }
        }

        SDL_RenderPresent(renderer);
    }
};

int main(int argc, char *args[])
{
    Board b;
    SDL view;

    {
        b.cells[11].val = 5;
        b.cells[12].val = 3;
        b.cells[14].val = 6;
        b.cells[18].val = 9;
        b.cells[19].val = 8;
        b.cells[22].val = 7;
        b.cells[24].val = 1;
        b.cells[25].val = 9;
        b.cells[26].val = 5;
        b.cells[38].val = 6;
        b.cells[41].val = 8;
        b.cells[44].val = 4;
        b.cells[47].val = 7;
        b.cells[52].val = 6;
        b.cells[54].val = 8;
        b.cells[56].val = 3;
        b.cells[58].val = 2;
        b.cells[63].val = 3;
        b.cells[66].val = 1;
        b.cells[69].val = 6;
        b.cells[72].val = 6;
        b.cells[84].val = 4;
        b.cells[85].val = 1;
        b.cells[86].val = 9;
        b.cells[88].val = 8;
        b.cells[91].val = 2;
        b.cells[92].val = 8;
        b.cells[96].val = 5;
        b.cells[98].val = 7;
        b.cells[99].val = 9;
    }

    int error = view.Setup();
    if (error)
        return error;

    SDL_Event e;
    bool quit = false;
    while (!quit)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN)
            {
                b.PromoteAnySingles();
            }
            if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                b.RemoveObviousImpossibilities();
            }
        }

        view.Render(b);
    }

    return 0;
}