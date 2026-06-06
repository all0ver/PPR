#!/usr/bin/perl
use strict;
use warnings;
use IO::Socket::INET;
use MIME::Base64;
use Digest::MD5 qw(md5_hex);

$| = 1; # wyłaczenie buforowania wyjścia - wypisywanie w czasie rzeczywistym

my $udp_port = 6000; # port socket/udp

# utworzenie gniazda udp
my $socket = IO::Socket::INET->new(
    LocalPort => $udp_port,
    Proto     => 'udp'
) or die "[Perl]: Nie mozna utworzyc gniazda UDP: $!\n";

# print "[Perl]: Oczekiwanie na paczki UDP na porcie $udp_port...\n\n";

my $expected_seq = 0; # oczekiwany numer pakietu

while (1) {
    my $datagram; # zmienna na otrzymane dane
    
    $socket->recv($datagram, 2048); # odbieranie danych
    
    my ($seq_num, $data) = split(/\|/, $datagram, 2); # rozdzielenie nagłówka od danych

    # sprawdzenie czy rozdzielenie się udało
    if (defined $seq_num && defined $data) {

        $socket->send("ACK|$seq_num"); # odesłanie ack

        # potwierdzenie, że otrzymana paczka to ta, która była oczekiwana
        if ($seq_num == $expected_seq) {
            
            if ($data eq "EOF") {
                print "\n[Perl] EOF\n";
                $expected_seq = 0; # reset licznika
            } else {
				my $md5_hash = md5_hex($data);
				# print "$md5_hash\n";
                my $raw_bytes = decode_base64($data); # dekodowanie otrzymanych danych z base64
				print $raw_bytes;
                my $hex_string = unpack("H*", $raw_bytes); #zamiana otrzymanych danych ba hex
                
                # print $hex_string;
                
                $expected_seq++; # zwiększenie licznika oczekiwanego pakietu
            }
        }
    }
}
