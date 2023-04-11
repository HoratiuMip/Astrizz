#define GTL_OS_WINDOWS
#define GTL_UNIQUE_SURFACE
#define GTL_SURFACE_TRACE_LENGTH 20
#define GTL_LOG
#define GTL_SHELL_COPY_DETECTOR

#include "C:\\Hackerman\\Git\\Nine_tailed\\GentleCore.cpp"

using namespace Gtl;


#define RND rand()


#pragma region Background

class Background : public Thing {
public:
    static inline const char*    path_sprite   = "Bits\\Background\\background.png";

    static inline double   max_offset    = 0.3;
    static inline double   max_shake     = 0.06;

public:
    Background( Renderer_view render )
        : _sprite( make< Sprite >( render, path_sprite ) )
    {
        _sprite.height_to_keep( Env :: H() * ( 1.0 + max_offset + max_shake ) );
    } 

private:
    Sprite   _sprite   = {};

    double   _shake    = 0.0;
    Vec2d    _offset   = {};

public:
    void render( Renderer_view render ) override {
        render( _sprite, Vec2d :: O().polar( RND % 360, _shake ) - _offset );
    }

    void refresh( double elapsed ) override {
        _shake = std :: max( 0.0, _shake - 0.02 * elapsed );
    }

public:
    Vec2d offset() const {
        return _offset;
    }

public:
    Background& shake( double value ) {
        _shake = std :: min( Env :: H() * max_shake, value );

        return *this;
    }

    Background& offset_to( Vec2d vec ) {
        _offset = vec;

        return *this;
    }

};

#pragma endregion Background



#pragma region Entities

template< typename T >
requires (
    std :: is_same_v< T, Sprite >
    ||
    std :: is_same_v< T, Sprites >
)
class Entity : public Thing {
public:
    Entity() = default;

    Entity( Renderer_view render, std :: string_view sprite_path, std :: string_view clust_path )
        : _sprite( make< T >( render, sprite_path ) ),
          _clust( Clust2d :: from_file( clust_path ) )
    {}

protected:
    T         _sprite   = {};
    Clust2d   _clust    = {};

public:
    Clust2d& operator () () {
        return _clust;
    } 

};



class Ship : public Entity< Sprite > {
public:
    static inline const struct {
        const char* sprite;
        const char* clust;
    } paths[] = { 
        { 
            sprite: "Bits\\Ships\\Redster\\redster.png",
            clust: "Bits\\Ships\\Redster\\redster.clst2.gtl"
        }
    };

public:
    Ship() = default;

    Ship( Renderer_view render, Background& bg )
        : Entity( render, paths[ 0 ].sprite, paths[ 0 ].clust ),
          _bg( &bg )
    {}

protected:
    Vec2d         _dest   = {};

    Background*   _bg     = NULL;

public:
    Ship& dest_to( Vec2d vec ) {
        _dest = vec;

        return *this;
    }

    Ship& dest_to_ms( Vec2d vec ) {
        return dest_to( vec * ( 1.0 - Background :: max_offset ) + _bg -> offset() );
    }

public:
    void render( Renderer_view render ) override {
        render( _sprite, _clust() );
    }

    void refresh( double elapsed ) override {
        Vec2d& org = _clust();

        _sprite << _clust.spin_at( Mouse :: vec()( org ).angel() );

        org.polar( _dest( org ).angel(), org.dist_to( _dest ) / 24.0 * elapsed );

        
        _bg -> offset_to( Vec2d :: O().polar( org.angel(), org.mag() * Background :: max_offset ) );
    }

};

#pragma endregion Entities



int main() {
    volatile bool ON = true;

    srand( Clock :: UNIX() );


    Surface surf = make< Surface >( 
        "Astrizz", 
        Coord< int >(), 
        Size< int >( Env :: W(), Env :: H() ) 
    );

    Renderer render = make< Renderer >( surf );


    std :: list< Gtl :: Thing* > things = {};


    Background background( render );
    things.push_back( &background );

    Ship ship( render, background );
    things.push_back( &ship );


    surf.on< KEY >( [ & ] ( Key key, Word state, Surface :: Trace& trace ) -> void {
        switch( key ) {
            case Key :: ESC: {
                if( state == UP )
                    ON = false;

                break; }


            case Key :: RMB: {
                ship.dest_to_ms( surf.ms_vec() );

                break; }


            case Key :: G: {
                if( Key :: all_down( Key :: D, Key :: B ) )
                    background.shake( 4.0 );

                break; }
        }
    } );

    surf.on< MOUSE >( [ & ] ( Vec2d vec, Vec2d last, Surface :: Trace& trace ) -> void {
        if( Key :: down( Key :: RMB ) )
            ship.dest_to_ms( vec );
    } );


    while( ON ) {
        static Clock clock = {};

        double elapsed = clock.lap() * 100.0;


        for( auto thing : things )
            thing -> refresh( elapsed );


        render.begin();

        for( auto thing : things )
            render( *thing );

        render.end();
    }


    return 0;
}
