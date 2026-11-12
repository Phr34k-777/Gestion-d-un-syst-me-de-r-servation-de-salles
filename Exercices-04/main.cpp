#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// Énumération pour les catégories de salles
enum class CategorieSalle {
    COURS,
    TD,
    SOUTENANCE
};

// Classe Salle
class Salle {
private:
    std::string numero;
    int capacite;
    CategorieSalle categorie;

public:
    Salle(std::string num, int cap, CategorieSalle cat)
        : numero(num), capacite(cap), categorie(cat) {}

    std::string getNumero() const { return numero; }
    int getCapacite() const { return capacite; }
    CategorieSalle getCategorie() const { return categorie; }
};

// Classe Enseignant
class Enseignant {
private:
    int id;
    std::string nom;
    std::string prenom;

public:
    Enseignant(int id, std::string nom, std::string prenom)
        : id(id), nom(nom), prenom(prenom) {}

    int getId() const { return id; }
    std::string getNomComplet() const { return prenom + " " + nom; }
};

// Classe Reservation
class Reservation {
private:
    int id;
    Salle* salle;
    Enseignant* enseignant;
    std::string date; // Format YYYY-MM-DD
    int heureDebut;   // Ex: 8 pour 08h00
    int heureFin;     // Ex: 10 pour 10h00

public:
    Reservation(int id, Salle* s, Enseignant* e, std::string d, int hDebut, int hFin)
        : id(id), salle(s), enseignant(e), date(d), heureDebut(hDebut), heureFin(hFin) {}

    int getId() const { return id; }
    Salle* getSalle() const { return salle; }
    Enseignant* getEnseignant() const { return enseignant; }
    std::string getDate() const { return date; }

    // Vérifie s'il y a un conflit horaire avec une autre demande
    bool chevauche(const std::string& numSalle, const std::string& d, int hDebut, int hFin) const {
        if (salle->getNumero() != numSalle || date != d) {
            return false;
        }
        // Condition de chevauchement de deux intervalles [A, B] et [C, D] : A < D et C < B
        return (heureDebut < hFin) && (hDebut < heureFin);
    }

    void afficher() const {
        std::cout << "Resa #" << id << " | Salle: " << salle->getNumero()
                  << " | Date: " << date << " (" << heureDebut << "h-" << heureFin << "h)"
                  << " | Par: " << enseignant->getNomComplet() << std::endl;
    }
};

// Classe Contrôleur : SystemeReservation
class SystemeReservation {
private:
    std::vector<Salle> salles;
    std::vector<Reservation> reservations;
    int prochainIdReservation = 1;

    Salle* trouverSalle(const std::string& numero) {
        for (auto& s : salles) {
            if (s.getNumero() == numero) return &s;
        }
        return nullptr;
    }

public:
    void ajouterSalle(const Salle& salle) {
        salles.push_back(salle);
    }

    // 1. Vérifier la disponibilité d'une salle
    bool verifierDisponibilite(const std::string& numSalle, const std::string& date, int hDebut, int hFin) const {
        for (const auto& res : reservations) {
            if (res.chevauche(numSalle, date, hDebut, hFin)) {
                return false; // Conflit détecté
            }
        }
        return true; // Disponible
    }

    // 2. Enregistrer une réservation (en empêchant les conflits)
    bool reserverSalle(Enseignant* enseignant, const std::string& numSalle, const std::string& date, int hDebut, int hFin) {
        if (hDebut >= hFin) {
            std::cout << "[Erreur] L'heure de debut doit etre inferieure a l'heure de fin.\n";
            return false;
        }

        Salle* salle = trouverSalle(numSalle);
        if (!salle) {
            std::cout << "[Erreur] La salle " << numSalle << " n'existe pas.\n";
            return false;
        }

        if (!verifierDisponibilite(numSalle, date, hDebut, hFin)) {
            std::cout << "[Conflit] La salle " << numSalle << " est deja reservee sur ce creneau (" 
                      << date << " de " << hDebut << "h a " << hFin << "h).\n";
            return false;
        }

        reservations.emplace_back(prochainIdReservation++, salle, enseignant, date, hDebut, hFin);
        std::cout << "[Succes] Reservation enregistree pour " << enseignant->getNomComplet() << ".\n";
        return true;
    }

    // 3. Annuler une réservation
    bool annulerReservation(int idReservation) {
        auto it = std::remove_if(reservations.begin(), reservations.end(),
            [idReservation](const Reservation& r) { return r.getId() == idReservation; });

        if (it != reservations.end()) {
            reservations.erase(it, reservations.end());
            std::cout << "[Succes] Reservation #" << idReservation << " annulee.\n";
            return true;
        }
        std::cout << "[Erreur] Reservation #" << idReservation << " introuvable.\n";
        return false;
    }

    void afficherToutesReservations() const {
        std::cout << "\n--- Liste des reservations ---\n";
        if (reservations.empty()) {
            std::cout << "Aucune reservation en cours.\n";
        }
        for (const auto& r : reservations) {
            r.afficher();
        }
        std::cout << "------------------------------\n\n";
    }
};

// Programme principal (Tests)
int main() {
    SystemeReservation systeme;

    // Ajout des salles
    systeme.ajouterSalle(Salle("Amphi A", 200, CategorieSalle::COURS));
    systeme.ajouterSalle(Salle("B102", 35, CategorieSalle::TD));

    // Création des enseignants
    Enseignant prof1(1, "Dupont", "Jean");
    Enseignant prof2(2, "Martin", "Claire");

    // Tests de réservation
    systeme.reserverSalle(&prof1, "B102", "2026-11-20", 8, 10);  // Succès
    systeme.reserverSalle(&prof2, "B102", "2026-11-20", 9, 11);  // Échec (chevauchement 9h-10h)
    systeme.reserverSalle(&prof2, "B102", "2026-11-20", 10, 12); // Succès (juste après)

    systeme.afficherToutesReservations();

    // Annulation et nouvelle tentative
    systeme.annulerReservation(1);
    systeme.reserverSalle(&prof2, "B102", "2026-11-20", 8, 10);  // Succès maintenant que le créneau est libre

    systeme.afficherToutesReservations();

    return 0;
}
