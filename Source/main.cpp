#define GTL_OS_WINDOWS
#define GTL_UNIQUE_SURFACE
#define GTL_SURFACE_TRACE_LENGTH 20
#define GTL_LOG
#define GTL_SHELL_COPY_DETECTOR

#include "C:\\Hackerman\\Git\\Nine_tailed\\GentleCore.cpp"



using namespace Gtl;


#define RND rand()


typedef   std :: vector< Sprite<> >   Sprites;

struct Package {
    mutable Sprites*     cntr      = NULL;
    mutable Clust2d      clst      = {};
    std :: string_view   sprites   = {};
    std :: string_view   clust     = {};
    std :: string_view   snd       = {};
};



class Background : public Entity {
public:
    static inline const char*    path_sprite   = "Bits\\Background\\background.png";

    static inline double   max_offset    = 0.3;
    static inline double   max_shake     = 0.06;

public:
    Background( View< Renderer > render )
        : _sprite( make< Sprite >( render, path_sprite ) )
    {
        _sprite.height_to_keep( Env :: H() * ( 1.0 + max_offset + max_shake ) );
    }

private:
    Sprite<>   _sprite   = {};

    double     _shake    = 0.0;
    Vec2d      _offset   = {};

public:
    void render( View< Renderer > render ) const override {
        render( _sprite, Vec2d :: O().polar( RND % 360, _shake ) - _offset );
    }

    void render( View< Renderer > render, Vec2d vec ) const override {}

public:
    void refresh( double elapsed ) override {
        _shake = std :: max( 0.0, _shake - 0.028 * elapsed );
    }

public:
    Vec2d offset() const {
        return _offset;
    }

public:
    Background& shake( double value ) {
        if( value <= _shake ) return *this;

        _shake = std :: min( Env :: H() * max_shake, value );

        return *this;
    }

    Background& shake_to( double value ) {
        _shake = std :: min( Env :: H() * max_shake, value );

        return *this;
    }

    Background& offset_to( Vec2d vec ) {
        _offset = vec;

        return *this;
    }

};



class Destroyable : public Entity {
public:
    Destroyable() = default;

    Destroyable( const Package& package )
        : Entity( package.clst )
    {
        _sprites.reserve( package.cntr -> size() );

        for( auto& spr : *package.cntr )
            _sprites.emplace_back( make< Sprite >( spr ) );
    }

protected:
    std :: vector< Sprite<> >   _sprites     = {};

    Clock                       _clock       = {};
    bool                        _destroyed   = false;
    std :: size_t               _at          = 0;

public:
    View< Sprite > active() {
        return _sprites.front();
    }

public:
    void destroy() {
        if( _destroyed ) return;

        _destroyed = true;

        _at = 1;

        _clock.lap();
    }

    bool done() {
        return _at >= _sprites.size();
    }

    void reset() {
        _destroyed = false;
        _at = 0;
    }

public:
    virtual void refresh( double elapsed ) override {
        if( !_destroyed ) return;

        if( _clock.peek_lap() > 0.06 ) {
            ++_at;

            _clock.lap();
        }
    }

public:
    virtual void render( View< Renderer > render ) const override {
        this -> render( render, _clust() );
    }

    virtual void render( View< Renderer > render, Vec2d vec ) const override {
        if( std :: size_t at = _at; at < _sprites.size() )
            render( _sprites[ at ], vec );
    }


};



class Ship : public Destroyable {
public:
    static inline std :: unique_ptr< Sprites[] >   sprites   = {};

    static inline Sound<>   explosion_sound   = {};

    static inline const Package packages[] = {
        {
            sprites: "Bits\\Ships\\Redster\\redster.sprs.gtl",
            clust: "Bits\\Ships\\Redster\\redster.clst2.gtl",
            snd: "Bits\\Ships\\Redster\\ameno.wav"
        },
        {
            sprites: "Bits\\Ships\\Whity\\whity.sprs.gtl",
            clust: "Bits\\Ships\\Whity\\whity.clst2.gtl",
            snd: "Bits\\Ships\\Whity\\ameno.wav"
        }
    };

public:
    Ship() = default;

    Ship( Background& bg )
        : Destroyable( packages[ RND % std :: size( packages ) ] ),
          _bg( &bg )
    {}

protected:
    int           _health   = 10;
    Vec2d         _dest     = {};
    Background*   _bg       = NULL;
    bool          _dog      = false;

public:
    Ship& dest_to( Vec2d vec ) {
        _dest = vec;

        return *this;
    }

    Ship& dest_to_ms( Vec2d vec ) {
        return dest_to( vec * ( 1.0 - Background :: max_offset ) + _bg -> offset() );
    }

public:
    const Clust2d& clust() {
        return _clust;
    }

    Background& bg() {
        return *_bg;
    }

public:
    virtual void render( View< Renderer > render ) const override {
        Destroyable :: render( render );
    }

public:
    virtual void refresh( double elapsed ) override {
        if( _health == 0 ) {
            if( !explosion_sound.is_playing() )
                explosion_sound.play();

            destroy();
        }

        Vec2d& org = _clust.origin_ref();

        if( done() )
            org = { 10000, 10000 };

        Destroyable :: refresh( elapsed );

        if( _destroyed ) return;

        active() << _clust.spin_at( Mouse :: vec()( org ).angel() );

        org.approach( _dest, org.dist_to( _dest ) / 24.0 * elapsed );


        _bg -> offset_to( Vec2d :: O().polar( org.angel(), org.mag() * Background :: max_offset ) );
    }

public:
    void operator -= ( int value ) {
        if( _dog ) return;

        _health = std :: max( 0, _health - value );
    }

    void operator += ( int value ) {
        _health = std :: min( 10, _health + value );
    }

    int health() const {
        return _health;
    }

    void swap_dog() {
        _dog ^= true;
    }

    void reset() {
        _health = 10;

        Destroyable :: reset();

        _clust = _dest = Vec2d :: O();

        explosion_sound.stop();
    }

};



class Ship_damager : public Destroyable {
public:
    Ship_damager() = default;

    Ship_damager( Ship* ship, const Package& package )
        : Destroyable( package ),
          _ship( ship )
    {
        _clust = Vec2d( 0.0, Env :: H_D() + 300.0 ).spin( RND % 360 );

        _angel = _clust().spin( 120.0 + RND % 121 )( _clust() ).angel();
    }

protected:
    Ship*    _ship    = NULL;

    double   _angel   = 0.0;

public:
    virtual void damage() = 0;

public:
    virtual void refresh( double elapsed ) override {
        Destroyable :: refresh( elapsed );

        if( _ship -> clust().X< bool >( Clust2d( _clust ) = _clust() - _ship -> bg().offset() ) ) {
            damage();

            destroy();
        }

        if( _clust().dist_to( Vec2d :: O() ) > 2000.0 )
            destroy();
    }

public:
    virtual void render( View< Renderer > render ) const override {
        Destroyable :: render( render, _clust() - _ship -> bg().offset() );
    }

};



class Missle : public Ship_damager {
public:
    static inline std :: unique_ptr< Sprites[] >   sprites   = {};

    static inline Sound<>   explosion_sound   = {};

    static inline const Package packages[] = {
        {
            sprites: "Bits\\Damagers\\Missle\\missle.sprs.gtl",
            clust: "Bits\\Damagers\\Missle\\missle.clst2.gtl",
            snd: "Bits\\Damagers\\Missle\\explosion.wav"
        }
    };

public:
    Missle() = default;

    Missle( Ship* ship )
        : Ship_damager( ship, packages[ RND % std :: size( packages ) ] )
    {
        active().spin_at( _angel + 180.0 ) >> _clust;
    }

public:
    virtual void damage() override {
        if( _destroyed ) return;

        _ship -> bg().shake( 4.0 );

        ( *_ship ) -= 1;

        explosion_sound.play();
    }

public:
    virtual void refresh( double elapsed ) override {
        Ship_damager :: refresh( elapsed );

        if( _destroyed ) return;

        Vec2d& org = _clust.origin_ref();

        org.polar( _angel, 10.0 * elapsed );
    }

};



class Asteroid : public Ship_damager {
public:
    static inline std :: unique_ptr< Sprites[] >   sprites   = {};

    static inline Sound<>   explosion_sound   = {};

    static inline const Package packages[] = {
        {
            sprites: "Bits\\Damagers\\Asteroid\\round.sprs.gtl",
            clust: "Bits\\Damagers\\Asteroid\\round.clst2.gtl",
            snd: "Bits\\Damagers\\Asteroid\\explosion.wav"

        },
        {
            sprites: "Bits\\Damagers\\Asteroid\\rectang.sprs.gtl",
            clust: "Bits\\Damagers\\Asteroid\\rectang.clst2.gtl",
            snd: "Bits\\Damagers\\Asteroid\\explosion.wav"
        }
    };

public:
    Asteroid() = default;

    Asteroid( Ship* ship )
        : Ship_damager( ship, packages[ RND % std :: size( packages ) ] )
    {
        active().scale_at( 0.3 + ( static_cast< double >( RND ) / RAND_MAX ) * 0.3 ) >> _clust;
    }

public:
    virtual void damage() override {
        if( _destroyed ) return;

        _ship -> bg().shake( 9.0 );

        ( *_ship ) -= 2;

        explosion_sound.play();
    }

public:
    virtual void refresh( double elapsed ) override {
        Ship_damager :: refresh( elapsed );

        if( _destroyed ) return;

        Vec2d& org = _clust.origin_ref();

        org.polar( _angel, 5.0 * elapsed );

        active().spin_with(
            static_cast< int >( _angel ) % 2 ? -1.0 : 1.0
            *
            1.6 * elapsed
        ) >> _clust;
    }

};



class Healpack : public Ship_damager {
public:
    static inline std :: unique_ptr< Sprites[] >   sprites   = {};

    static inline Sound<>   explosion_sound   = {};

    static inline const Package packages[] = {
        {
            sprites: "Bits\\Damagers\\Healpack\\healpack.sprs.gtl",
            clust: "Bits\\Damagers\\Healpack\\healpack.clst2.gtl",
            snd: "Bits\\Damagers\\Healpack\\explosion.wav"
        }
    };

public:
    Healpack() = default;

    Healpack( Ship* ship )
        : Ship_damager( ship, packages[ RND % std :: size( packages ) ] )
    {}

public:
    virtual void damage() override {
        if( _destroyed ) return;

        ( *_ship ) += 3;

        explosion_sound.play();
    }

public:
    virtual void refresh( double elapsed ) override {
        Ship_damager :: refresh( elapsed );

        if( _destroyed ) return;

        Vec2d& org = _clust.origin_ref();

        org.polar( _angel, 6.0 * elapsed );

        active().spin_with( 5.0 * elapsed ) >> _clust;
    }

};




template< typename T >
void init( View< Renderer > render, Audio<>& audio ) {
    T :: sprites = std :: make_unique< Sprites[] >( std :: size( T :: packages ) );

    for( std :: size_t idx = 0; idx < std :: size( T :: packages ); ++idx ) {
        T :: sprites[ idx ] = Sprite<> :: from_file_m( render, T :: packages[ idx ].sprites );

        T :: packages[ idx ].cntr = &T :: sprites[ idx ];

        T :: packages[ idx ].clst = Clust2d :: from_file( T :: packages[ idx ].clust );

        T :: explosion_sound = make< Sound >( audio, T :: packages[ idx ].snd.data() );
    }
}



#define ENTITYN

int main() {
    volatile bool ON = true;

    srand( Clock :: UNIX() );


    Audio<> audio = make< Audio >( Audio<> :: devices()[ 0 ], 44100, 1, 16, 256 );


    Surface<> surf = make< Surface >(
        "Astrizz",
        Coord< int >(),
        Size< int >( Env :: W(), Env :: H() )
    );

    Renderer<> render = make< Renderer >( surf );


    init< Ship >( render, audio );
    init< Missle >( render, audio );
    init< Asteroid >( render, audio );
    init< Healpack >( render, audio );


    std :: list< std :: unique_ptr< Ship_damager > > damagers = {};


    Background background( render );

    Ship ship( background );


#if defined( ENTITY )

    std :: list< Entity* > entities = {};

    entities.push_back( &background );
    entities.push_back( &ship );

#else
    std :: list< Rendable* > rendables = {};
    std :: list< Refreshable* > refreshables = {};

    rendables.push_back( &background );
    rendables.push_back( &ship );

    refreshables.push_back( &background );
    refreshables.push_back( &ship );

#endif

    Sprite<> health_ball = make< Sprite >( render, "Bits\\Misc\\health_ball.png" )
    .pin_to( Sprite<> :: TOP_LEFT );


    auto sensor = Sensor_with< Sprite<> > :: from_file( surf, render, "Bits\\Sensors\\Kaboom\\kaboom.snsr" );

    Sound<> tremble = make< Sound >( audio, "Bits\\Misc\\commence.wav" )
    .volume_to( 0.5 );

    sensor.wake();


    std :: binary_semaphore sem( 0 );

    sensor.on< Sensor::UP >( [ & ] ( Vec2d v1, Vec2d v2 ) -> void {
        sem.release();
    } );

    sensor.on< Sensor::BEGIN >( [ & ] ( Vec2d v1, Vec2d v2 ) -> void {
        background.shake_to( 6.0 );
    } );

    sensor.on< Sensor::END >( [ & ] ( Vec2d v1, Vec2d v2 ) -> void {
        background.shake_to( 0.0 );
    } );

    while( !sem.try_acquire() ) {
        if( sensor.is_hovered() )
            tremble.play();

        render.begin()( background )( sensor ).end();
    }

    sensor.ko();


    int   speed   = 121;


    surf.on< Surface<> :: KEY >( [ & ] ( Key key, Key :: State state, Surface<> :: Trace& trace ) -> void {
        switch( key ) {
            case Key :: ESC: {
                if( state == Key :: UP )
                    ON = false;

                break; }


            case Key :: RMB: {
                ship.dest_to_ms( surf.ms_vec() );

                break; }


            case Key :: G: {
                if( Key :: all_down( Key :: D, Key :: B ) )
                    background.shake( 4.0 ), ship.destroy();

                break; }

            case Key :: X: {
                if( state == Key :: UP )
                    ship.swap_dog();

                break; }

            case Key :: R: {
                if( state == Key :: State :: DOWN ) return;

                for( auto& damager : damagers )
                    damager -> destroy();

                ship.reset();

                break; }

            case Key :: AUP: {
                speed = std :: max( 1, speed - 10 );

                break; }

            case Key :: ADOWN: {
                speed += 10;

                break; }
        }
    } );

    surf.on< Surface<> :: MOUSE >( [ & ] ( Vec2d vec, Vec2d last, Surface<> :: Trace& trace ) -> void {
        if( Key :: down( Key :: RMB ) )
            ship.dest_to_ms( vec );
    } );

    surf.on< Surface<> :: SCROLL >( [ & ] ( Scroll :: Dir dir, Surface<> :: Trace& trace ) -> void {
        if( Key :: down( Key :: CTRL ) ) {
            if( dir == Scroll :: UP )
                speed = std :: max( 1, speed - 10 );
            else
                speed += 10;
        }

    } );



    while( ON ) {
        static Clock clock = {};

        double elapsed = clock.lap() * 100.0;


        render.begin();



        if( RND % speed == 0 ) {
            switch( RND % 7 ) {
                case 0:
                case 1:
                case 2:
                case 3: damagers.emplace_front( new Missle( &ship ) ); break;
                case 4:
                case 5: damagers.emplace_front( new Asteroid( &ship ) ); break;
                case 6: damagers.emplace_front( new Healpack( &ship ) ); break;
            }
        }


    #if defined( ENTITY )

        for( auto entity : entities ) {
            entity -> refresh( elapsed );

            render( *entity );
        }

    #else

        for( auto entity : refreshables )
            entity -> refresh( elapsed );

        for( auto entity : rendables )
            render( *entity );

    #endif


        damagers.remove_if( [ & ] ( auto& damager ) -> bool {
            damager -> refresh( elapsed );

            render( *damager );

            return damager -> done();
        } );


        for( uint8_t ball = 0; ball < ship.health(); ++ball )
            render(
                health_ball,
                surf.pull_vec( Coord< float >() ) >> ( ball * ( health_ball.width() + 10 ) )
            );


        render.end();
    }


    return 0;
}
