struct clockWord
{
    int from;
    int to;
};

clockWord Het = {0, 3};
clockWord Is = {4, 6};

clockWord HourWords[12] =
    {{51, 54},
     {55, 59},
     {62, 66},
     {66, 70},
     {70, 74},
     {74, 77},
     {77, 82},
     {88, 92},
     {83, 88},
     {91, 95},
     {96, 99},
     {99, 105}};

clockWord Vijf = {7, 11};
clockWord Tien = {11, 15};
clockWord FirstVoor = {18, 22};
clockWord FirstOver = {22, 26};
clockWord Kwart = {28, 33};
clockWord Half = {33, 37};
clockWord SecondOver = {40, 44};
clockWord SecondVoor = {44, 48};
clockWord Uur = {107, 110};
clockWord Stop = {0, 0};

clockWord MinutesSentences[12][3] =
    {
        {Uur, Stop},
        {Vijf, FirstOver, Stop},
        {Tien, FirstOver, Stop},
        {Kwart, SecondOver, Stop},
        {Tien, FirstVoor, Half},
        {Vijf, FirstVoor, Half},
        {Half, Stop},
        {Vijf, FirstOver, Half},
        {Tien, FirstOver, Half},
        {Kwart, SecondVoor, Stop},
        {Tien, FirstVoor, Stop},
        {Vijf, FirstVoor, Stop}};

int Wifi[4] = {16, 12, 10, 8};