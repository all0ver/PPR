#!/usr/bin/php
<?php
    $port   = 12345; // port do xml-rpc
    $host   = '127.0.0.1';
    
    // rozmiar przesyłanych danych
    $chunk_size = 768; // 768 / 3 = 256 ;; 256*4 = 1024 (3 bajty (znaki) zmieniają się na 4 znaki base64, czyli odczytanie 768 znaków to 1024 bajty base64


    $stdin = fopen('php://stdin', 'rb'); // otwarcie stdin do odczytu (r) binarnie (b)
    if (!$stdin) {
        die("Blad: Nie mozna otworzyc STDIN!\n");
    }

    echo "\n[PHP] przesyłanie xml-rpc";

    while (!feof($stdin)) {
        
        $dane = fread($stdin, $chunk_size); // odczytanie danych z stdin
        
        if ($dane === false || strlen($dane) === 0) {
            $b64 = "EOF";
            echo "\n[PHP] Koniec danych";
        } else {
			$b64 = base64_encode($dane); // zapakowanie do base64
		}
       
        // md5
         $checksum = md5($b64);
         # echo "\n$checksum";

        
        echo "[PHP] Wysylam";
        echo strlen($b64);

        // budowa zapytania xml-rpc
        $req = xmlrpc_encode_request("data.transfer", array($b64)); // wykorzystanie funkcji data.transfer, która jest zaimplementowana w c
        // konfiguracja przesyłania danych: post, xml, z danymi $req
        $ctx = stream_context_create(array(
            'http' => array(
                'method'    => "POST",
                'header'    => array("Content-Type: text/xml"),
                'content'   => $req
            )
        ));

        // wyslanie zapytnia xml-rpc do c
        $xml = @file_get_contents("http://$host:$port/RPC2", false, $ctx); //@ chowa błędy, błędy wypisuje if niżej
        
        if ($xml === false) {
            die("\nBlad: Zerwano polaczenie z serwerem C\n");
        }

		$res = xmlrpc_decode($xml); // odkodowanie odpowiedzi
		// w przypadku błędu (is_array zwróci fałsz, gdy odbierze string ok od C - ominięcie if)
		if (is_array($res) && xmlrpc_is_fault($res)) {
			print "\nXML-RPC Blad\n";
			exit(1);
		}
		
		// jeżeli koniec danych to koniec pętli
		if ($b64 === "EOF") { break; }
    }

    fclose($stdin); // zamknięcie stdin
?>
