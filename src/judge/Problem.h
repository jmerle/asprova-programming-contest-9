#pragma once


#include <tuple>
#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>
#include <set>
#include <numeric>
#include <cmath>
#include <queue>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>
#include <random>
#include <map>
#include <chrono>
#include <limits>

const double PI = acos( -1 );
constexpr int HOUR = 3600;
constexpr int DAY = 86400;
constexpr int WEEK = DAY * 7;

template<class... Ts>
void dump( const Ts&... b )
{
    ( std::cerr << ... << ( std::cerr << ' ', b ) );
    std::cerr << '\n';
}



using namespace std;



struct Rand
{
    std::mt19937_64 rand;


    explicit Rand( unsigned long long seed ) :rand( seed )
    {}

    unsigned int randint()
    {
        for( int i = 0; i < rand() % 30ull; i++ ) // skip
            rand();

        return static_cast<unsigned int>(rand() % numeric_limits<unsigned int>::max());
    }

    // [0,r)�͈̔͂̐����ŗ������� ... generate random integer in [0,r)
    unsigned int randint( unsigned int r )
    {
        assert( r != 0 );
        return randint() % r;
    }

    // [l,r)�͈̔͂̐����ŗ������� ... generate random integer in [l,r)
    unsigned int randint( unsigned int l, unsigned int r )
    {
        assert( r > l );
        return l + randint( r - l );
    }

    unsigned int randint( pair<int, int>lr )
    {
        assert( lr.second > lr.first );
        return lr.first + randint( lr.second - lr.first );
    }

    // ����n�̏���������_���ɐ������A���̑Ok�����o�͂��� ... generate a random permutation of size n, and return the first k
    vector<int> randperm( int n, int k )
    {
        assert( k >= 0 && k <= n );
        vector<int> ret( n );
        for( int i = 0; i < n; i++ )
        {
            ret[i] = i;
        }
        for( int i = 0; i < k; i++ )
        {
            swap( ret[i], ret[randint( i, n )] );
        }
        return vector<int>( ret.begin(), ret.begin() + k );
    }


    // [0,1]�͈̔͂̎����ŗ������� ... generate random real number in [0,1]
    double uniform()
    {
        static std::uniform_real_distribution<> dist( 0.0, 1.0 );
        return dist( rand );
    }

    // [0,r]�͈̔͂̎����ŗ������� ... generate random real number in [0,r]
    double uniform( double r )
    {
        assert( r >= 0.0 );
        return uniform() * r;
    }

    // [l,r]�͈̔͂̎����ŗ������� ... generate random real number in [l,r]
    double uniform( double l, double r )
    {
        assert( r >= l );
        return l + uniform( r - l );
    }

    // normal distribution (�� = mean, �� = sigma)
    double normal( double mean, double sigma )
    {
        double x = sqrt( -2.0 * log( uniform() ) ) * sin( 2.0 * PI * uniform() );
        return mean + sigma * x;
    }

    // normal distribution (�� = 0, �� = sigma)
    double normal( double sigma )
    {
        double x = sqrt( -2.0 * log( uniform() ) ) * sin( 2.0 * PI * uniform() );
        return sigma * x;
    }

    // �v�f[l,r)����Ȃ钷��n�̐���𐶐� ... Generate a sequence of numbers of length n consisting of elements [l,r)
    vector<int> randseq( int n, int l, int r )
    {
        vector<int> ret( n );
        for( int i = 0; i < n; i++ )
        {
            ret[i] = randint( l, r );
        }
        return ret;
    }

    // n�̏��񂩂�d������k�v�f�����o��...Take out k elements without duplication from permutation of n
    vector<int> divide( int n, int k )
    {
        set<int>st;
        while( st.size() < k )
        {
            st.insert( randint() % n );
        }
        return vector<int>( st.begin(), st.end() );
    }

    // [1,v.size())�̏��񂩂�v[i]���d�݂Ƃ��ďd������,ok�Ɋ܂܂��k�v�f�����o��
    // From the permutation of [1, v.size()), take out k elements contained in candidate without duplication with v[i] as the weight
    const double EPS = 1e-9;
    vector<int> weighted_unique_seq( vector<double>& v, int k, set<int>* candidate = nullptr )
    {
        if( candidate != nullptr )
            assert( v.size() >= candidate->size() );

        double sum = std::accumulate( v.begin(), v.end(), 0.0 );
        set<int> st;
        while( st.size() < k )
        {
            double th = uniform( 0, sum );
            double t_sum = 0;
            for( int i = 0; i < (int)v.size(); i++ )
            {
                t_sum += v[i];
                if( t_sum + EPS > th )
                {
                    if( candidate == nullptr )
                        st.insert( i );
                    else if( candidate != nullptr && candidate->count( i ) )
                        st.insert( i );

                    break;
                }
            }
        }
        return vector<int>( st.begin(), st.end() );
    }
};

constexpr int CalendarTypeN = 9;

struct CalendarType
{
    std::vector< std::vector<pair<int, int>> > pattern = {
            {{}}, // pattern 1
            {{ {9 * HOUR,12 * HOUR} } }, // pattern 2
            {{ {13 * HOUR,18 * HOUR} } }, // pattern 3
            {{ {9 * HOUR,12 * HOUR}, {13 * HOUR,18 * HOUR, } } }, // pattern 4
            {{ {9 * HOUR,12 * HOUR}, {13 * HOUR,20 * HOUR, } } }, // pattern 5
            {{ {9 * HOUR,12 * HOUR}, {13 * HOUR,22 * HOUR, } } }, // pattern 6
            {{ {9 * HOUR,12 * HOUR}, {13 * HOUR,24 * HOUR, } } }, // pattern 7
            {{ {9 * HOUR,12 * HOUR}, {13 * HOUR,26 * HOUR, } } }, // pattern 8
            {{ {9 * HOUR,12 * HOUR}, {13 * HOUR,28 * HOUR, } } }, // pattern 9

    };

    std::vector<double> Time = { 0.0, 3.0 / 8, 5.0 / 8, 1.0, 1.25, 1.5, 1.75, 2.0, 2.25 }; // 8���Ԃ���Ƃ����Ƃ��̊��� ... Percentage based on 8 hours

    vector<int> totalTimeA = { 0,3,5,8,10,12,12,12,12 }; // ���΂̍��v ... total day shift
    vector<int> totalTimeB = { 0,0,0,0, 0, 0, 2, 4, 6 }; // ��΂̍��v ... total night shift
    vector<int> totalTime = { 0,3,5,8,10,12,14,16,18 }; // �Ζ����Ԃ̍��v ... total working time

    void addCalendar( vector<pair<int, int>>& calendar, int week, int typeA, int typeB )
    {
        int offset = WEEK * week;
        for( int i = 0; i < 5; i++ )
        {
            for( auto& e : pattern[typeA] )
            {
                calendar.push_back( { e.first + offset, e.second + offset } );
            }
            offset += DAY;
        }
        for( int i = 0; i < 2; i++ )
        {
            for( auto& e : pattern[typeB] )
            {
                calendar.push_back( { e.first + offset, e.second + offset } );
            }
            offset += DAY;
        }
    }
};

struct Parameter
{
    int itemMin = 30;
    int itemMax = 300;
    int itemProcNMin = 1;

    int resMin = 10;
    int resMax = 20;
    double calendar1CostRatioMin = 0.5;
    double calendar1CostRatioMax = 1.0;
    double resInitCalendarMutationRatioMin = 0.0;
    double resInitCalendarMutationRatioMax = 0.2;

    int weeksMin = 8;
    int weeksMax = 16;

    int procNMin = 2;

    double workerNSigma = 2;
    int baseCostPerHour = 800;
    double costPerHourSigma = 500;
    double costPerHourNightSigma = 50;

    int prodTimeBase = 3600; // 1hour
    int prodTimeSigmaMin = 1000;
    int prodTimeSigmaMax = 2000;

    double prodTimeVarMin = 0.8;
    double prodTimeVarMax = 1.2;

    int changeLimitMin = 2;
    int changeLimitMax = 8;

    double costExpMin = 1.1;
    double costExpMax = 1.5;

    long long maxCost = 10'000'000'000LL;

    vector<int> reactiveN = { 50, 100, 300 };
};

static CalendarType Calendar;
static Parameter param;


class ProblemVar
{
public: // ���͐����p�p�����[�^ ... Parameters for generating input
    int procN;  // �H����ސ� ... Number of types of process
    vector<double> procDemand; // �H�����v(����i�ڂ��H����ʂ�m��) ... Process demand (probability of being selected for a certain item's process)
    vector<double> resDemandMutationRatio; // �������v�̕ω��� ... Rate of change in resource demand
    vector < set<int> > procResSet; //

    double addCostHoliday = 1.2 / 5.0 * 2.0; // �x���̒ǉ��R�X�g ... Additional holiday costs

    vector<string> originalCalendar; // the calendar to use for generating input

public:
    struct Item
    {
        int itemNo;
        int itemProcN; // �H���� ... number of process
        vector<int> proc; // i�Ԗڂ̍H���̎��� ... resources of the i-th process
        pair<int, int> prodTimeRange;
    };

    struct Resource
    {
        int resNo;
        int procNo;

        double costRatio = 1.0;
        double calendar1CostRatio = 1.0;
        double calendar0CostRatio = 0;

        // ���͐����p ... Parameters for generating input
        vector<double> calendarTypeXRatio;
        int workerN; // �ғ��ɕK�v�ȍ�ƎҐ� ... required workers
        int costPerHour;  // ��ƈ��̎���
        int costPerHourNight; // ��Ύ���
        double resDemand = 0.0; // �H�����v(���̍H����ʂ�i�ڂ����������m��)
    };

    vector< vector<pair<int, int>> >calendar; // i�Ԗڂ̎����̉ғ����Ԃ́C[calendar[i][j].first, calendar[i][j].second)�̃��X�g�ŕ\����� ... The working time of the i-th resource is represented by the list [calendar[i][j].first, calendar[i][j].second).

    struct Operation
    {
        int opNo;
        int itemNo;
        std::vector<int> prodTime;
        int let;
    };

    int itemN; // ���i�� ... Number of Items
    int resourceN; // ������ ... Number of Resources
    int operationN; // ��Ɛ� ... Number of Operations
    vector<Item> itemList;
    vector<Resource> resourceList;
    vector<Operation> opList;


    map<tuple<int, int>, int> costTypeA; // �����̉ғ��p�^�[���̃R�X�g ... Cost of working pattern on weekdays
    map<tuple<int, int>, int> costTypeB; // �x���̉ғ��p�^�[���̃R�X�g ... Cost of working pattern on holiday

    int week;
    int resCalendarChangeLimitN;
    int reactiveN;

    // ���͐����̂��߂̃p�����^
    int inputNo;
    string input_INPUT;
    string input_outputfile_name = "";
    bool generated = false;

    void Output();
    void Input( istream& );
};


