struct control {
    int mode;          
    int duration;     
    int type;        
    int packet_len;
    int reserved;
};

#define TYPE_UDP 1
#define TYPE_TCP 2
#define MODE_TX 1
#define MODE_RX 2
