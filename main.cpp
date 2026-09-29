// Pong - SDL2
// A small Pong game with a scoreboard, CPU opponent (3 difficulties), local
// two-player mode, keyboard controls and on-screen (mouse / touch) controls.
//
// Build:  g++ -std=c++17 -O2 main.cpp -o pong $(sdl2-config --cflags --libs)

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// Layout (logical pixels - the window can be resized, everything scales)
// ---------------------------------------------------------------------------
constexpr int SCREEN_W = 800;
constexpr int SCREEN_H = 640;
constexpr int HUD_H = 70;                  // scoreboard strip at the top
constexpr int FIELD_X = 0;
constexpr int FIELD_Y = HUD_H;
constexpr int FIELD_W = SCREEN_W;
constexpr int FIELD_H = 480;
constexpr int BAR_Y = FIELD_Y + FIELD_H;   // on-screen control bar below the field

// ---------------------------------------------------------------------------
// Gameplay tuning (speeds are in pixels per second)
// ---------------------------------------------------------------------------
constexpr float PADDLE_W = 14.0f;
constexpr float PADDLE_H = 90.0f;
constexpr float PADDLE_MARGIN = 30.0f;
constexpr float PADDLE_SPEED = 520.0f;
constexpr float BALL_SIZE = 12.0f;
constexpr float BALL_START_SPEED = 380.0f;
constexpr float BALL_MAX_SPEED = 950.0f;
constexpr float BALL_SPEEDUP = 1.06f;      // multiplier on every paddle hit
constexpr float MAX_BOUNCE_ANGLE = 1.05f;  // ~60 degrees, when hitting the paddle edge
constexpr float SERVE_DELAY = 1.0f;        // seconds before the ball is served
constexpr int WIN_SCORE = 7;
constexpr int TRAIL_LEN = 12;

// ---------------------------------------------------------------------------
// Colors
// ---------------------------------------------------------------------------
struct Color { Uint8 r, g, b, a; };

constexpr Color COL_BG{10, 12, 26, 255};
constexpr Color COL_FIELD{16, 20, 40, 255};
constexpr Color COL_LINE{60, 70, 110, 255};
constexpr Color COL_TEXT{235, 240, 255, 255};
constexpr Color COL_DIM{120, 130, 170, 255};
constexpr Color COL_BUTTON{28, 34, 64, 255};
constexpr Color COL_P1{0, 220, 255, 255};
constexpr Color COL_P2{255, 80, 140, 255};
constexpr Color COL_BALL{255, 225, 90, 255};

Color withAlpha(Color c, int a) { return {c.r, c.g, c.b, static_cast<Uint8>(std::clamp(a, 0, 255))}; }

Color mix(Color a, Color b, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    auto lerp = [t](Uint8 x, Uint8 y) { return static_cast<Uint8>(x + (y - x) * t); };
    return {lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b), lerp(a.a, b.a)};
}

void setColor(SDL_Renderer* r, Color c) { SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a); }

void fillRect(SDL_Renderer* r, float x, float y, float w, float h, Color c)
{
    setColor(r, c);
    SDL_Rect rc{static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)),
                static_cast<int>(std::lround(w)), static_cast<int>(std::lround(h))};
    SDL_RenderFillRect(r, &rc);
}

// ---------------------------------------------------------------------------
// Tiny built-in 5x7 bitmap font, so no SDL_ttf / font file is needed
// ---------------------------------------------------------------------------
struct Glyph { char ch; const char* rows[7]; };

const Glyph FONT[] = {
    {'A', {" ### ", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}},
    {'B', {"#### ", "#   #", "#   #", "#### ", "#   #", "#   #", "#### "}},
    {'C', {" ####", "#    ", "#    ", "#    ", "#    ", "#    ", " ####"}},
    {'D', {"#### ", "#   #", "#   #", "#   #", "#   #", "#   #", "#### "}},
    {'E', {"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#####"}},
    {'F', {"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#    "}},
    {'G', {" ####", "#    ", "#    ", "#  ##", "#   #", "#   #", " ####"}},
    {'H', {"#   #", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}},
    {'I', {"#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "#####"}},
    {'J', {"  ###", "   # ", "   # ", "   # ", "#  # ", "#  # ", " ##  "}},
    {'K', {"#   #", "#  # ", "# #  ", "##   ", "# #  ", "#  # ", "#   #"}},
    {'L', {"#    ", "#    ", "#    ", "#    ", "#    ", "#    ", "#####"}},
    {'M', {"#   #", "## ##", "# # #", "# # #", "#   #", "#   #", "#   #"}},
    {'N', {"#   #", "##  #", "# # #", "#  ##", "#   #", "#   #", "#   #"}},
    {'O', {" ### ", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}},
    {'P', {"#### ", "#   #", "#   #", "#### ", "#    ", "#    ", "#    "}},
    {'Q', {" ### ", "#   #", "#   #", "#   #", "# # #", "#  # ", " ## #"}},
    {'R', {"#### ", "#   #", "#   #", "#### ", "# #  ", "#  # ", "#   #"}},
    {'S', {" ####", "#    ", "#    ", " ### ", "    #", "    #", "#### "}},
    {'T', {"#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  "}},
    {'U', {"#   #", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}},
    {'V', {"#   #", "#   #", "#   #", "#   #", "#   #", " # # ", "  #  "}},
    {'W', {"#   #", "#   #", "#   #", "# # #", "# # #", "## ##", "#   #"}},
    {'X', {"#   #", "#   #", " # # ", "  #  ", " # # ", "#   #", "#   #"}},
    {'Y', {"#   #", "#   #", " # # ", "  #  ", "  #  ", "  #  ", "  #  "}},
    {'Z', {"#####", "    #", "   # ", "  #  ", " #   ", "#    ", "#####"}},
    {'0', {" ### ", "#   #", "#  ##", "# # #", "##  #", "#   #", " ### "}},
    {'1', {"  #  ", " ##  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "}},
    {'2', {" ### ", "#   #", "    #", "   # ", "  #  ", " #   ", "#####"}},
    {'3', {"#####", "   # ", "  #  ", "   # ", "    #", "#   #", " ### "}},
    {'4', {"   # ", "  ## ", " # # ", "#  # ", "#####", "   # ", "   # "}},
    {'5', {"#####", "#    ", "#### ", "    #", "    #", "#   #", " ### "}},
    {'6', {"  ## ", " #   ", "#    ", "#### ", "#   #", "#   #", " ### "}},
    {'7', {"#####", "    #", "   # ", "  #  ", " #   ", " #   ", " #   "}},
    {'8', {" ### ", "#   #", "#   #", " ### ", "#   #", "#   #", " ### "}},
    {'9', {" ### ", "#   #", "#   #", " ####", "    #", "   # ", " ##  "}},
    {':', {"     ", "  #  ", "  #  ", "     ", "  #  ", "  #  ", "     "}},
    {'-', {"     ", "     ", "     ", "#####", "     ", "     ", "     "}},
    {'.', {"     ", "     ", "     ", "     ", "     ", " ##  ", " ##  "}},
    {'!', {"  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "     ", "  #  "}},
    {'/', {"    #", "    #", "   # ", "  #  ", " #   ", "#    ", "#    "}},
    {'?', {" ### ", "#   #", "    #", "   # ", "  #  ", "     ", "  #  "}},
    {'+', {"     ", "  #  ", "  #  ", "#####", "  #  ", "  #  ", "     "}},
};

const Glyph* findGlyph(char c)
{
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    for (const Glyph& g : FONT)
        if (g.ch == c) return &g;
    return nullptr;  // space / unknown characters are left blank
}

int textWidth(const std::string& s, int scale)
{
    return s.empty() ? 0 : static_cast<int>(s.size()) * 6 * scale - scale;
}

void drawText(SDL_Renderer* r, const std::string& s, int x, int y, int scale, Color c)
{
    std::vector<SDL_Rect> pixels;
    for (size_t i = 0; i < s.size(); ++i) {
        const Glyph* g = findGlyph(s[i]);
        if (!g) continue;
        const int gx = x + static_cast<int>(i) * 6 * scale;
        for (int row = 0; row < 7; ++row)
            for (int col = 0; col < 5; ++col)
                if (g->rows[row][col] == '#')
                    pixels.push_back({gx + col * scale, y + row * scale, scale, scale});
    }
    setColor(r, c);
    SDL_RenderFillRects(r, pixels.data(), static_cast<int>(pixels.size()));
}

void drawTextCentered(SDL_Renderer* r, const std::string& s, int cx, int y, int scale, Color c)
{
    drawText(r, s, cx - textWidth(s, scale) / 2, y, scale, c);
}

void drawTextRight(SDL_Renderer* r, const std::string& s, int right, int y, int scale, Color c)
{
    drawText(r, s, right - textWidth(s, scale), y, scale, c);
}

// Filled isosceles triangle (used for the arrow buttons).
void fillTriangle(SDL_Renderer* r, int cx, int cy, int size, bool pointsUp, Color c)
{
    setColor(r, c);
    for (int i = 0; i < size; ++i) {
        const int half = static_cast<int>((i + 0.5f) / size * size * 0.6f);
        const int y = pointsUp ? cy - size / 2 + i : cy + size / 2 - i;
        SDL_RenderDrawLine(r, cx - half, y, cx + half, y);
    }
}

// ---------------------------------------------------------------------------
// On-screen buttons
// ---------------------------------------------------------------------------
enum ButtonId { BTN_P1_UP, BTN_P1_DOWN, BTN_PAUSE, BTN_RESTART, BTN_MODE, BTN_P2_UP, BTN_P2_DOWN, BTN_COUNT };

constexpr int BTN_Y = BAR_Y + 10;
constexpr int BTN_H = 56;

const SDL_Rect BUTTON_RECTS[BTN_COUNT] = {
    {16, BTN_Y, 80, BTN_H},    // P1 up
    {104, BTN_Y, 80, BTN_H},   // P1 down
    {210, BTN_Y, 120, BTN_H},  // start / pause / resume
    {340, BTN_Y, 120, BTN_H},  // restart
    {470, BTN_Y, 120, BTN_H},  // mode
    {616, BTN_Y, 80, BTN_H},   // P2 up
    {704, BTN_Y, 80, BTN_H},   // P2 down
};

bool pointInRect(float x, float y, const SDL_Rect& rc)
{
    return x >= rc.x && x < rc.x + rc.w && y >= rc.y && y < rc.y + rc.h;
}

// ---------------------------------------------------------------------------
// Game
// ---------------------------------------------------------------------------
enum class State { Ready, Serving, Playing, Paused, GameOver };
enum Mode { MODE_EASY, MODE_NORMAL, MODE_HARD, MODE_2P, MODE_COUNT };

const char* modeLabel(Mode m)
{
    switch (m) {
    case MODE_EASY: return "1P EASY";
    case MODE_NORMAL: return "1P NORMAL";
    case MODE_HARD: return "1P HARD";
    default: return "2 PLAYER";
    }
}

struct Paddle {
    float x = 0, y = 0;     // top-left corner
    int score = 0;
    float hitFlash = 0;     // 1 -> 0 after the ball hits the paddle
    float scoreFlash = 0;   // 1 -> 0 after this side scores
};

struct Ball {
    float x = 0, y = 0;     // top-left corner
    float vx = 0, vy = 0;
    float speed = BALL_START_SPEED;
};

// Per-frame movement request for one paddle.
struct PaddleInput {
    int dir = 0;            // -1 up, +1 down
    bool dragging = false;  // a finger / mouse is dragging this paddle
    float dragY = 0;        // desired paddle center
};

class Game {
public:
    Game() : rng_(std::random_device{}()) { toReady(); }

    State state() const { return state_; }
    Mode mode() const { return mode_; }
    bool twoPlayer() const { return mode_ == MODE_2P; }

    // Space / Enter / the big center button.
    void primaryAction()
    {
        switch (state_) {
        case State::Ready:
        case State::GameOver: newMatch(); break;
        case State::Serving:
        case State::Playing: pause(); break;
        case State::Paused: state_ = resumeState_; break;
        }
    }

    void pause()
    {
        if (state_ == State::Serving || state_ == State::Playing) {
            resumeState_ = state_;
            state_ = State::Paused;
        }
    }

    void restart() { newMatch(); }

    void cycleMode()
    {
        mode_ = static_cast<Mode>((mode_ + 1) % MODE_COUNT);
        toReady();
    }

    void update(float dt, const PaddleInput& in1, const PaddleInput& in2)
    {
        for (Paddle* p : {&p1_, &p2_}) {
            p->hitFlash = std::max(0.0f, p->hitFlash - dt * 4.0f);
            p->scoreFlash = std::max(0.0f, p->scoreFlash - dt * 1.5f);
        }
        if (state_ == State::Paused) return;

        movePaddle(p1_, in1, dt);
        if (twoPlayer())
            movePaddle(p2_, in2, dt);
        else
            updateAi(dt);

        if (state_ == State::Serving) {
            serveTimer_ -= dt;
            if (serveTimer_ <= 0.0f) launchBall();
        } else if (state_ == State::Playing) {
            stepBall(dt);
            pushTrail();
        }
    }

    void render(SDL_Renderer* r, const bool pressed[BTN_COUNT]) const
    {
        setColor(r, COL_BG);
        SDL_RenderClear(r);
        renderField(r);
        renderBall(r);
        renderPaddle(r, p1_, COL_P1);
        renderPaddle(r, p2_, COL_P2);
        renderHud(r);
        renderOverlay(r);
        renderControls(r, pressed);
    }

private:
    // --- state changes ----------------------------------------------------
    void centerPaddles()
    {
        p1_.x = FIELD_X + PADDLE_MARGIN;
        p2_.x = FIELD_X + FIELD_W - PADDLE_MARGIN - PADDLE_W;
        p1_.y = p2_.y = FIELD_Y + (FIELD_H - PADDLE_H) / 2;
    }

    void centerBall()
    {
        ball_.x = FIELD_X + (FIELD_W - BALL_SIZE) / 2;
        ball_.y = FIELD_Y + (FIELD_H - BALL_SIZE) / 2;
        ball_.vx = ball_.vy = 0;
        ball_.speed = BALL_START_SPEED;
        trailCount_ = 0;
    }

    void toReady()
    {
        p1_.score = p2_.score = 0;
        centerPaddles();
        centerBall();
        state_ = State::Ready;
    }

    void newMatch()
    {
        p1_.score = p2_.score = 0;
        p1_.scoreFlash = p2_.scoreFlash = 0;
        centerPaddles();
        serve(std::uniform_int_distribution<int>(0, 1)(rng_) ? 1 : -1);
    }

    // Put the ball in the middle and send it towards `dir` after a short delay.
    void serve(int dir)
    {
        centerBall();
        serveDir_ = dir;
        serveTimer_ = SERVE_DELAY;
        state_ = State::Serving;
    }

    void launchBall()
    {
        const float angle = std::uniform_real_distribution<float>(-0.45f, 0.45f)(rng_);
        ball_.speed = BALL_START_SPEED;
        ball_.vx = serveDir_ * ball_.speed * std::cos(angle);
        ball_.vy = ball_.speed * std::sin(angle);
        pickAiError();
        state_ = State::Playing;
    }

    void pointScored(Paddle& scorer, int serveTowards)
    {
        ++scorer.score;
        scorer.scoreFlash = 1.0f;
        if (scorer.score >= WIN_SCORE) {
            winner_ = (&scorer == &p1_) ? 1 : 2;
            centerBall();
            state_ = State::GameOver;
        } else {
            serve(serveTowards);  // the side that conceded receives the serve
        }
    }

    // --- movement ---------------------------------------------------------
    static void clampPaddle(Paddle& p)
    {
        p.y = std::clamp(p.y, static_cast<float>(FIELD_Y), FIELD_Y + FIELD_H - PADDLE_H);
    }

    static void movePaddle(Paddle& p, const PaddleInput& in, float dt)
    {
        if (in.dragging)
            p.y = in.dragY - PADDLE_H / 2;
        else
            p.y += in.dir * PADDLE_SPEED * dt;
        clampPaddle(p);
    }

    void pickAiError()
    {
        static const float maxError[] = {55.0f, 30.0f, 10.0f};
        const float e = maxError[std::min<int>(mode_, MODE_HARD)];
        aiError_ = std::uniform_real_distribution<float>(-e, e)(rng_);
    }

    // Where will the ball's center be when it reaches x = targetX (bouncing off walls)?
    float predictBallCenterY(float targetX) const
    {
        const float t = (targetX - (ball_.x + BALL_SIZE)) / ball_.vx;
        if (t <= 0.0f) return ball_.y + BALL_SIZE / 2;
        const float minY = FIELD_Y;
        const float span = FIELD_H - BALL_SIZE;
        float rel = std::fmod(ball_.y + ball_.vy * t - minY, 2 * span);
        if (rel < 0) rel += 2 * span;
        if (rel > span) rel = 2 * span - rel;
        return minY + rel + BALL_SIZE / 2;
    }

    void updateAi(float dt)
    {
        static const float aiSpeed[] = {240.0f, 360.0f, 520.0f};
        const int level = std::min<int>(mode_, MODE_HARD);

        float target = FIELD_Y + FIELD_H / 2.0f;  // drift back to the middle when idle
        if (state_ == State::Playing && ball_.vx > 0) {
            target = (mode_ == MODE_EASY) ? ball_.y + BALL_SIZE / 2 : predictBallCenterY(p2_.x);
            target += aiError_;
        }
        const float diff = target - (p2_.y + PADDLE_H / 2);
        const float maxStep = aiSpeed[level] * dt;
        if (std::fabs(diff) > 4.0f) p2_.y += std::clamp(diff, -maxStep, maxStep);
        clampPaddle(p2_);
    }

    bool ballOverlaps(const Paddle& p) const
    {
        return ball_.x < p.x + PADDLE_W && ball_.x + BALL_SIZE > p.x &&
               ball_.y < p.y + PADDLE_H && ball_.y + BALL_SIZE > p.y;
    }

    // Bounce off a paddle. The further from the paddle center, the steeper the angle.
    void bounce(Paddle& p, int dir)
    {
        float rel = ((ball_.y + BALL_SIZE / 2) - (p.y + PADDLE_H / 2)) / (PADDLE_H / 2 + BALL_SIZE / 2);
        rel = std::clamp(rel, -1.0f, 1.0f);
        const float angle = rel * MAX_BOUNCE_ANGLE;
        ball_.speed = std::min(ball_.speed * BALL_SPEEDUP, BALL_MAX_SPEED);
        ball_.vx = dir * ball_.speed * std::cos(angle);
        ball_.vy = ball_.speed * std::sin(angle);
        ball_.x = dir > 0 ? p.x + PADDLE_W : p.x - BALL_SIZE;
        p.hitFlash = 1.0f;
        if (dir > 0) pickAiError();  // heading towards the CPU again
    }

    void stepBall(float dt)
    {
        // Move in small sub-steps so a fast ball can never tunnel through a paddle.
        const float dist = std::hypot(ball_.vx, ball_.vy) * dt;
        const int steps = std::max(1, static_cast<int>(std::ceil(dist / (BALL_SIZE * 0.5f))));
        const float h = dt / steps;

        for (int i = 0; i < steps; ++i) {
            ball_.x += ball_.vx * h;
            ball_.y += ball_.vy * h;

            if (ball_.y < FIELD_Y) {
                ball_.y = FIELD_Y;
                ball_.vy = std::fabs(ball_.vy);
            } else if (ball_.y + BALL_SIZE > FIELD_Y + FIELD_H) {
                ball_.y = FIELD_Y + FIELD_H - BALL_SIZE;
                ball_.vy = -std::fabs(ball_.vy);
            }

            if (ball_.vx < 0 && ballOverlaps(p1_))
                bounce(p1_, +1);
            else if (ball_.vx > 0 && ballOverlaps(p2_))
                bounce(p2_, -1);

            if (ball_.x + BALL_SIZE < FIELD_X) {
                pointScored(p2_, -1);
                return;
            }
            if (ball_.x > FIELD_X + FIELD_W) {
                pointScored(p1_, +1);
                return;
            }
        }
    }

    void pushTrail()
    {
        trail_[trailHead_] = {ball_.x, ball_.y};
        trailHead_ = (trailHead_ + 1) % TRAIL_LEN;
        trailCount_ = std::min(trailCount_ + 1, TRAIL_LEN);
    }

    // --- rendering --------------------------------------------------------
    void renderField(SDL_Renderer* r) const
    {
        fillRect(r, FIELD_X, FIELD_Y, FIELD_W, FIELD_H, COL_FIELD);
        fillRect(r, FIELD_X, FIELD_Y, FIELD_W, 2, COL_LINE);
        fillRect(r, FIELD_X, FIELD_Y + FIELD_H - 2, FIELD_W, 2, COL_LINE);
        for (int y = FIELD_Y + 10; y < FIELD_Y + FIELD_H - 10; y += 28)
            fillRect(r, FIELD_X + FIELD_W / 2 - 2, y, 4, 16, COL_LINE);
    }

    static void renderPaddle(SDL_Renderer* r, const Paddle& p, Color c)
    {
        for (int g = 3; g >= 1; --g) {
            const float pad = g * 3.0f;
            fillRect(r, p.x - pad, p.y - pad, PADDLE_W + 2 * pad, PADDLE_H + 2 * pad,
                     withAlpha(c, static_cast<int>(18 + p.hitFlash * 50)));
        }
        fillRect(r, p.x, p.y, PADDLE_W, PADDLE_H, mix(c, COL_TEXT, p.hitFlash * 0.8f));
    }

    void renderBall(SDL_Renderer* r) const
    {
        if (state_ == State::Ready || state_ == State::GameOver) return;
        // Blink while waiting to be served.
        if (state_ == State::Serving && std::fmod(serveTimer_, 0.3f) > 0.15f) return;

        for (int i = 0; i < trailCount_; ++i) {
            const int idx = (trailHead_ - trailCount_ + i + TRAIL_LEN) % TRAIL_LEN;
            const float t = static_cast<float>(i + 1) / (trailCount_ + 1);
            const float s = BALL_SIZE * (0.35f + 0.5f * t);
            const float off = (BALL_SIZE - s) / 2;
            fillRect(r, trail_[idx].x + off, trail_[idx].y + off, s, s,
                     withAlpha(COL_BALL, static_cast<int>(120 * t)));
        }
        fillRect(r, ball_.x - 3, ball_.y - 3, BALL_SIZE + 6, BALL_SIZE + 6, withAlpha(COL_BALL, 50));
        fillRect(r, ball_.x, ball_.y, BALL_SIZE, BALL_SIZE, COL_BALL);
    }

    void renderHud(SDL_Renderer* r) const
    {
        const bool twoP = twoPlayer();
        drawText(r, twoP ? "PLAYER 1" : "PLAYER", 24, 14, 3, COL_P1);
        drawText(r, modeLabel(mode_), 24, 46, 2, COL_DIM);
        drawTextRight(r, twoP ? "PLAYER 2" : "CPU", SCREEN_W - 24, 14, 3, COL_P2);
        drawTextRight(r, "FIRST TO " + std::to_string(WIN_SCORE), SCREEN_W - 24, 46, 2, COL_DIM);

        drawTextRight(r, std::to_string(p1_.score), SCREEN_W / 2 - 28, 10, 7,
                      mix(COL_TEXT, COL_P1, p1_.scoreFlash));
        drawText(r, std::to_string(p2_.score), SCREEN_W / 2 + 28, 10, 7,
                 mix(COL_TEXT, COL_P2, p2_.scoreFlash));
        fillRect(r, SCREEN_W / 2 - 6, 32, 12, 4, COL_DIM);
    }

    void renderOverlay(SDL_Renderer* r) const
    {
        if (state_ == State::Serving || state_ == State::Playing) return;

        fillRect(r, FIELD_X, FIELD_Y, FIELD_W, FIELD_H, {0, 0, 0, 165});
        const int cx = FIELD_X + FIELD_W / 2;

        if (state_ == State::Ready) {
            drawTextCentered(r, "PONG", cx, FIELD_Y + 60, 12, COL_BALL);
            drawTextCentered(r, "MODE: " + std::string(modeLabel(mode_)), cx, FIELD_Y + 170, 3, COL_TEXT);
            drawTextCentered(r, "PRESS SPACE OR TAP START", cx, FIELD_Y + 215, 3, COL_BALL);

            const bool twoP = twoPlayer();
            int y = FIELD_Y + 285;
            const std::string lines[] = {
                twoP ? "PLAYER 1: W / S     PLAYER 2: UP / DOWN" : "MOVE: W / S  OR  UP / DOWN",
                "OR HOLD THE ARROW BUTTONS / DRAG ON THE FIELD",
                "SPACE/P: PAUSE   R: RESTART   M: MODE",
                "F11: FULLSCREEN   ESC: QUIT",
            };
            for (const std::string& line : lines) {
                drawTextCentered(r, line, cx, y, 2, COL_DIM);
                y += 30;
            }
        } else if (state_ == State::Paused) {
            drawTextCentered(r, "PAUSED", cx, FIELD_Y + 170, 8, COL_TEXT);
            drawTextCentered(r, "PRESS SPACE OR TAP RESUME", cx, FIELD_Y + 250, 3, COL_BALL);
        } else if (state_ == State::GameOver) {
            const bool twoP = twoPlayer();
            std::string title;
            Color color = COL_P1;
            if (winner_ == 1) {
                title = twoP ? "PLAYER 1 WINS!" : "YOU WIN!";
            } else {
                title = twoP ? "PLAYER 2 WINS!" : "CPU WINS!";
                color = COL_P2;
            }
            drawTextCentered(r, title, cx, FIELD_Y + 140, 7, color);
            drawTextCentered(r, std::to_string(p1_.score) + " - " + std::to_string(p2_.score), cx,
                             FIELD_Y + 220, 5, COL_TEXT);
            drawTextCentered(r, "PRESS SPACE OR TAP AGAIN", cx, FIELD_Y + 300, 3, COL_BALL);
        }
    }

    void renderControls(SDL_Renderer* r, const bool pressed[BTN_COUNT]) const
    {
        const bool twoP = twoPlayer();
        const char* primary = "PAUSE";
        if (state_ == State::Ready) primary = "START";
        else if (state_ == State::Paused) primary = "RESUME";
        else if (state_ == State::GameOver) primary = "AGAIN";

        for (int id = 0; id < BTN_COUNT; ++id) {
            const SDL_Rect& rc = BUTTON_RECTS[id];
            const bool enabled = !(id == BTN_P2_UP || id == BTN_P2_DOWN) || twoP;
            Color accent = COL_BALL;
            if (id == BTN_P1_UP || id == BTN_P1_DOWN) accent = COL_P1;
            if (id == BTN_P2_UP || id == BTN_P2_DOWN) accent = COL_P2;
            if (!enabled) accent = COL_LINE;

            const bool down = enabled && pressed[id];
            fillRect(r, rc.x, rc.y, rc.w, rc.h, down ? withAlpha(accent, 110) : COL_BUTTON);
            setColor(r, down ? accent : withAlpha(accent, enabled ? 200 : 255));
            SDL_RenderDrawRect(r, &rc);

            const Color fg = enabled ? COL_TEXT : COL_LINE;
            const int cx = rc.x + rc.w / 2, cy = rc.y + rc.h / 2;
            switch (id) {
            case BTN_P1_UP:
            case BTN_P2_UP: fillTriangle(r, cx, cy, 22, true, fg); break;
            case BTN_P1_DOWN:
            case BTN_P2_DOWN: fillTriangle(r, cx, cy, 22, false, fg); break;
            case BTN_PAUSE: drawTextCentered(r, primary, cx, cy - 7, 2, fg); break;
            case BTN_RESTART: drawTextCentered(r, "RESTART", cx, cy - 7, 2, fg); break;
            case BTN_MODE: drawTextCentered(r, modeLabel(mode_), cx, cy - 7, 2, fg); break;
            }
        }

        // Keyboard hints under each group of buttons.
        const int hy = BTN_Y + BTN_H + 7;
        drawTextCentered(r, twoP ? "W" : "W / UP", 56, hy, 1, COL_DIM);
        drawTextCentered(r, twoP ? "S" : "S / DOWN", 144, hy, 1, COL_DIM);
        drawTextCentered(r, "SPACE / P", 270, hy, 1, COL_DIM);
        drawTextCentered(r, "R", 400, hy, 1, COL_DIM);
        drawTextCentered(r, "M", 530, hy, 1, COL_DIM);
        if (twoP) {
            drawTextCentered(r, "UP", 656, hy, 1, COL_DIM);
            drawTextCentered(r, "DOWN", 744, hy, 1, COL_DIM);
        } else {
            drawTextCentered(r, "CPU CONTROLLED", 700, hy, 1, COL_DIM);
        }
    }

    std::mt19937 rng_;
    State state_ = State::Ready;
    State resumeState_ = State::Playing;
    Mode mode_ = MODE_NORMAL;
    Paddle p1_, p2_;
    Ball ball_;
    int serveDir_ = 1;
    float serveTimer_ = 0;
    int winner_ = 0;
    float aiError_ = 0;
    SDL_FPoint trail_[TRAIL_LEN] = {};
    int trailHead_ = 0;
    int trailCount_ = 0;
};

// ---------------------------------------------------------------------------
// Pointer (mouse + multi-touch) tracking for the on-screen controls
// ---------------------------------------------------------------------------
constexpr Sint64 MOUSE_POINTER = -1;
constexpr int TARGET_NONE = -1;
constexpr int TARGET_DRAG_P1 = 100;
constexpr int TARGET_DRAG_P2 = 101;

struct Pointer {
    int target = TARGET_NONE;
    float x = 0, y = 0;
};

class Pointers {
public:
    void press(Sint64 id, float x, float y, Game& game)
    {
        Pointer p{TARGET_NONE, x, y};
        for (int b = 0; b < BTN_COUNT; ++b) {
            if (!pointInRect(x, y, BUTTON_RECTS[b])) continue;
            if ((b == BTN_P2_UP || b == BTN_P2_DOWN) && !game.twoPlayer()) break;
            p.target = b;
            if (b == BTN_PAUSE) game.primaryAction();
            else if (b == BTN_RESTART) game.restart();
            else if (b == BTN_MODE) game.cycleMode();
            break;
        }
        if (p.target == TARGET_NONE && y >= FIELD_Y && y < FIELD_Y + FIELD_H)
            p.target = (game.twoPlayer() && x >= FIELD_X + FIELD_W / 2) ? TARGET_DRAG_P2 : TARGET_DRAG_P1;
        pointers_[id] = p;
    }

    void move(Sint64 id, float x, float y)
    {
        auto it = pointers_.find(id);
        if (it != pointers_.end()) {
            it->second.x = x;
            it->second.y = y;
        }
    }

    void release(Sint64 id) { pointers_.erase(id); }
    void clear() { pointers_.clear(); }

    // A hold button counts as held while a pointer that pressed it is still over it.
    bool held(int button) const
    {
        for (const auto& kv : pointers_)
            if (kv.second.target == button && pointInRect(kv.second.x, kv.second.y, BUTTON_RECTS[button]))
                return true;
        return false;
    }

    bool dragging(int target, float& y) const
    {
        for (const auto& kv : pointers_)
            if (kv.second.target == target) {
                y = kv.second.y;
                return true;
            }
        return false;
    }

private:
    std::map<Sint64, Pointer> pointers_;
};

void fingerToLogical(SDL_Window* window, SDL_Renderer* renderer, float nx, float ny, float& lx, float& ly)
{
    int ww = 0, wh = 0;
    SDL_GetWindowSize(window, &ww, &wh);
#if SDL_VERSION_ATLEAST(2, 0, 18)
    SDL_RenderWindowToLogical(renderer, static_cast<int>(nx * ww), static_cast<int>(ny * wh), &lx, &ly);
#else
    (void)renderer;
    lx = nx * SCREEN_W;
    ly = ny * SCREEN_H;
#endif
}

} // namespace

int main(int, char**)
{
    SDL_SetMainReady();
    // Touch and mouse are handled separately, so don't let SDL synthesize one from the other.
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Pong - SDL2 by SahajMansuri", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(renderer, SCREEN_W, SCREEN_H);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_RendererInfo info{};
    SDL_GetRendererInfo(renderer, &info);
    const bool vsync = (info.flags & SDL_RENDERER_PRESENTVSYNC) != 0;

    Game game;
    Pointers pointers;
    bool quit = false;
    const Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 prev = SDL_GetPerformanceCounter();

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT: quit = true; break;

            case SDL_KEYDOWN:
                if (e.key.repeat) break;
                switch (e.key.keysym.sym) {
                case SDLK_SPACE:
                case SDLK_RETURN:
                case SDLK_KP_ENTER: game.primaryAction(); break;
                case SDLK_p:
                    if (game.state() == State::Paused) game.primaryAction();
                    else game.pause();
                    break;
                case SDLK_r: game.restart(); break;
                case SDLK_m:
                case SDLK_TAB: game.cycleMode(); break;
                case SDLK_F11: {
                    const bool fs = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
                    SDL_SetWindowFullscreen(window, fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                    break;
                }
                case SDLK_ESCAPE:
                    // First Esc pauses a running game, a second one quits.
                    if (game.state() == State::Playing || game.state() == State::Serving) game.pause();
                    else quit = true;
                    break;
                default: break;
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (e.button.which != SDL_TOUCH_MOUSEID && e.button.button == SDL_BUTTON_LEFT)
                    pointers.press(MOUSE_POINTER, static_cast<float>(e.button.x), static_cast<float>(e.button.y), game);
                break;
            case SDL_MOUSEMOTION:
                if (e.motion.which != SDL_TOUCH_MOUSEID)
                    pointers.move(MOUSE_POINTER, static_cast<float>(e.motion.x), static_cast<float>(e.motion.y));
                break;
            case SDL_MOUSEBUTTONUP:
                if (e.button.which != SDL_TOUCH_MOUSEID && e.button.button == SDL_BUTTON_LEFT)
                    pointers.release(MOUSE_POINTER);
                break;

            case SDL_FINGERDOWN:
            case SDL_FINGERMOTION: {
                float x = 0, y = 0;
                fingerToLogical(window, renderer, e.tfinger.x, e.tfinger.y, x, y);
                if (e.type == SDL_FINGERDOWN) pointers.press(e.tfinger.fingerId, x, y, game);
                else pointers.move(e.tfinger.fingerId, x, y);
                break;
            }
            case SDL_FINGERUP: pointers.release(e.tfinger.fingerId); break;

            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                    game.pause();
                    pointers.clear();
                }
                break;
            default: break;
            }
        }

        const Uint64 now = SDL_GetPerformanceCounter();
        const float dt = std::min(static_cast<float>(now - prev) / freq, 0.05f);
        prev = now;

        // Combine keyboard and on-screen controls.
        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        const bool twoP = game.twoPlayer();
        const bool arrowUp = keys[SDL_SCANCODE_UP], arrowDown = keys[SDL_SCANCODE_DOWN];

        bool pressed[BTN_COUNT] = {};
        pressed[BTN_P1_UP] = keys[SDL_SCANCODE_W] || pointers.held(BTN_P1_UP) || (!twoP && arrowUp);
        pressed[BTN_P1_DOWN] = keys[SDL_SCANCODE_S] || pointers.held(BTN_P1_DOWN) || (!twoP && arrowDown);
        pressed[BTN_P2_UP] = twoP && (arrowUp || pointers.held(BTN_P2_UP));
        pressed[BTN_P2_DOWN] = twoP && (arrowDown || pointers.held(BTN_P2_DOWN));
        pressed[BTN_PAUSE] = pointers.held(BTN_PAUSE);
        pressed[BTN_RESTART] = pointers.held(BTN_RESTART);
        pressed[BTN_MODE] = pointers.held(BTN_MODE);

        PaddleInput in1, in2;
        in1.dir = pressed[BTN_P1_DOWN] - pressed[BTN_P1_UP];
        in1.dragging = pointers.dragging(TARGET_DRAG_P1, in1.dragY);
        in2.dir = pressed[BTN_P2_DOWN] - pressed[BTN_P2_UP];
        in2.dragging = twoP && pointers.dragging(TARGET_DRAG_P2, in2.dragY);

        game.update(dt, in1, in2);
        game.render(renderer, pressed);
        SDL_RenderPresent(renderer);

        if (!vsync) SDL_Delay(8);  // ~120 FPS cap when vsync is unavailable
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
